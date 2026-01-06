#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"

#include "user_credential_manager/user_credential_manager.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"

#include <atomic>
#include <string_view>

class WebServer
{
    public:

        static WebServer & GetInstance();

        bool Init(
            UserCredential::UserCredentialManager * pUserCredentialManager,
            WifiExtender::WifiExtenderIf * pWifiExtenderIf
        );

        void Startup();

        ~WebServer();

    private:

        WebServer():
            m_WebServerThreadRunning(false),
            m_WebServerInitialized(false),
            m_WebServerTaskHandle(nullptr)
        {};

        static constexpr int MONGOOSE_TASK_STACK_SIZE = 4096 * 2;

        static constexpr int MONGOOSE_TASK_PRIO = 3;

        static void WebServerMain(void *pArg);

        static constexpr std::string_view m_pTaskName = "WebServerTask";

        std::atomic_bool m_WebServerThreadRunning;
        std::atomic_bool m_WebServerInitialized;

        TaskHandle_t m_WebServerTaskHandle;

        static UserCredential::UserCredentialManager * m_pUserCredentialManager;
        static WifiExtender::WifiExtenderIf * m_pWifiExtender;
        
        static int AuthenticateUser(const char *user, const char *pass);
        
};