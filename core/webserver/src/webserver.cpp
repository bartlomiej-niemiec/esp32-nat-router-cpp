#include "webserver.hpp"
#include "webserver_srvs.hpp"
#include "esp_log.h"

void WebServer::WebServerMain(void *pArg) {

    WebServer * pInstance = reinterpret_cast<WebServer*>(pArg);

    ESP_LOGI("MONGOOSE", "run_mongoose started");
    mongoose_init();
    mongoose_set_auth_handler(WebServerServices::AuthenticateUser);
    mongoose_set_http_handlers("info", WebServerServices::GetWifiNatRouterInfo, WebServerServices::SetWifiNatRouterInfo);
    mongoose_set_http_handlers("stanetworks", WebServerServices::GetStaScannedNetworks, NULL);
    mongoose_set_http_handlers("scannetworks", WebServerServices::IsStaScannningInProgress, WebServerServices::StartStaScannningNetworks);
    mongoose_set_http_handlers("saveapsettings", WebServerServices::GetApSetting, WebServerServices::SetApSetting);
    mongoose_set_http_handlers("savestasettings", WebServerServices::GetStaSettings, WebServerServices::SetStaSetings);
    mongoose_set_http_handlers("applynetworkconfig", WebServerServices::IsApplyDisabled, WebServerServices::StartWifiNatRouterWithNewConfig);
    mongoose_set_http_handlers("login", WebServerServices::GetLogin, WebServerServices::SetLogin);
    mg_log_set(MG_LL_DEBUG);    // Set log level to debug
    for(;;) {                   // Infinite event loop
        mongoose_poll();        // Process network events
        WebServerServices::Update();
        WebServerServices::Refresh();
    }
}

WebServer & WebServer::GetInstance()
{
    static WebServer webserver;
    return webserver;
}


void WebServer::Startup(WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterAppIf)
{


    if (!m_WebServerThreadRunning)
    {
        assert(pWifiNatRouterAppIf != nullptr);
        m_pWifiNatRouterAppIf = pWifiNatRouterAppIf;
        m_WebServerThreadRunning = true;
        xTaskCreate(
            WebServerMain,
            m_pTaskName.data(),
            MONGOOSE_TASK_STACK_SIZE,
            this,
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