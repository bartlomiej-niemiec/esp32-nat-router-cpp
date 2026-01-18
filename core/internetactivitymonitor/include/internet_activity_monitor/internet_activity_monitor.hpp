#pragma once 

#include "wifi_nat_router_app_if.hpp"
#include "ping/ping_sock.h"
#include "esp_log.h"

class InternetActivityMonitor
{
    public:

        InternetActivityMonitor(const WifiNatRouterApp::WifiNatRouterAppIf & rWifiNatRouterIf);

        ~InternetActivityMonitor();

        bool Check();

    private:

        esp_ping_handle_t m_InternetAccessPingHandle;

        const WifiNatRouterApp::WifiNatRouterAppIf & m_rWifiNatRouterIf;

        bool m_runnning;

        int m_ping_timeout_count;

        int m_ping_success_count;

        static void on_ping_success_cb(esp_ping_handle_t hdl, void *args)
        {
            auto * pInstance = reinterpret_cast<InternetActivityMonitor*>(args);
            pInstance->m_ping_success_count++;
        }

        static void on_ping_timeout_cb(esp_ping_handle_t hdl, void *args)
        {
            auto * pInstance = reinterpret_cast<InternetActivityMonitor*>(args);
            pInstance->m_ping_timeout_count++;
        }

        static void on_ping_end_cb(esp_ping_handle_t hdl, void *args)
        {
            auto * pInstance = reinterpret_cast<InternetActivityMonitor*>(args);
            ESP_LOGI("InternetActivityMonitor", "Ping Success Count: %i, Ping Timeout Count: %i", pInstance->m_ping_success_count, pInstance->m_ping_timeout_count);
            
            /// m_rWifiNatRouterIf.SendCommand();
            pInstance->m_runnning = false;
        }

};