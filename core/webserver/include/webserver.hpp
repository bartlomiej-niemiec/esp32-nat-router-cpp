#pragma once

#include "wifi_nat_router_app_if.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"

#include <atomic>
#include <string_view>

class WebServer
{
    public:

        static WebServer & GetInstance();

        void Startup(WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterAppIf);

        ~WebServer();

    private:

        WebServer():
            m_WebServerThreadRunning(false),
            m_WebServerTaskHandle(nullptr)
        {};

        static constexpr int MONGOOSE_TASK_STACK_SIZE = 4096 * 2;

        static constexpr int MONGOOSE_TASK_PRIO = 3;

        static void WebServerMain(void *pArg);

        static constexpr std::string_view m_pTaskName = "WebServerTask";

        std::atomic_bool m_WebServerThreadRunning;

        WifiNatRouterApp::WifiNatRouterAppIf * m_pWifiNatRouterAppIf;

        TaskHandle_t m_WebServerTaskHandle;
        
        static int AuthenticateUser(const char *user, const char *pass);
        
};