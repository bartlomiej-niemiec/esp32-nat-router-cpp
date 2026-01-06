#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "wifi_extender_if/wifi_extender_factory.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_scanner_types.hpp"

#include "user_credential_manager/user_credential_manager.hpp"

#include "network_config_manager.hpp"
#include "network_status_led.hpp"
#include "webserver.hpp"

class Controller
{
    
    public:

        void Startup();

    private:

        NetworkStatusLed::NetworkStatusLed * m_Led;

        WifiExtender::WifiExtenderIf * m_pWifiExtender;

        NetworkConfigManager m_NetworkConfigManager;

        static constexpr uint32_t WIFI_EXTENDER_QUEUE_SIZE = 16;
        QueueHandle_t m_WifiExtenderEventQueue;
        class WifiEventDispatcher:
            public WifiExtender::EventListener
        {
            public:

                WifiEventDispatcher(QueueHandle_t q):
                    m_QueueHandle(q)
                {};

                void Callback(WifiExtender::WifiExtenderState event) override
                {
                    xQueueGenericSend(m_QueueHandle, (void * ) &event, ( TickType_t ) 0, queueSEND_TO_BACK );
                };
            
            private:
                QueueHandle_t m_QueueHandle; 
        };

};