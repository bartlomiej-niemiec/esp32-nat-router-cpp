#include "webserver.hpp"
#include "user_credential_manager/user_credential_manager.hpp"
#include "esp_log.h"

 UserCredential::UserCredentialManager * WebServer::m_pUserCredentialManager = nullptr;
 WifiExtender::WifiExtenderIf * WebServer::m_pWifiExtender = nullptr;

int WebServer::AuthenticateUser(const char *user, const char *pass)
{
    int userlevel = 0;
    std::vector<std::string> users = m_pUserCredentialManager->GetUserNames();
    ESP_LOGI("MONGOOSE", "User: %s, Password: %s", user, pass);
    auto it = std::find_if(users.begin(), users.end(), [&user](std::string & s){return strcmp(s.data(), user) == 0;});
    if (it != users.end())
    {
        ESP_LOGI("MONGOOSE", "User found");
        auto err = m_pUserCredentialManager->VerifyUserPassword(user, pass, userlevel);
        if (err.has_value())
        {
            ESP_LOGI("MONGOOSE", "Error: %i", static_cast<int>(err.value()));
            userlevel = 0;
        }
    }

    return userlevel;
}

void WebServer::WebServerMain(void *pArg) {
    ESP_LOGI("MONGOOSE", "run_mongoose started");
    mongoose_init();
    mongoose_set_auth_handler(AuthenticateUser);
    mg_log_set(MG_LL_DEBUG);  // Set log level to debug
    for(;;) {                // Infinite event loop
        mongoose_poll();   // Process network events
    }
}

WebServer & WebServer::GetInstance()
{
    static WebServer webserver;
    return webserver;
}

bool WebServer::Init(
    UserCredential::UserCredentialManager * pUserCredentialManager,
    WifiExtender::WifiExtenderIf * pWifiExtenderIf)
{
    if (!m_WebServerInitialized)
    {
        if (nullptr == pUserCredentialManager) return false;
        if (nullptr == pWifiExtenderIf) return false;

        m_pUserCredentialManager = pUserCredentialManager;
        m_pWifiExtender = pWifiExtenderIf;

        m_WebServerInitialized = true;
    }
    return m_WebServerInitialized;
}

void WebServer::Startup()
{
    if (!m_WebServerThreadRunning && m_WebServerInitialized)
    {
        m_WebServerThreadRunning = true;
        xTaskCreate(
            WebServerMain,
            m_pTaskName.data(),
            MONGOOSE_TASK_STACK_SIZE,
            nullptr,
            MONGOOSE_TASK_PRIO,
            &m_WebServerTaskHandle
        );
        assert(nullptr != m_WebServerTaskHandle);
    }
}

WebServer::~WebServer()
{
    if (m_WebServerTaskHandle)
    {
        m_WebServerThreadRunning = false;
        vTaskDelete(m_WebServerTaskHandle);
    }
}