#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "wifi_extender_if/wifi_extender_factory.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_scanner_types.hpp"

#include "rgbled_if/rgbled_if.hpp"
#include "rgbled_if/rgbled_factory.hpp"
#include "rgbled_if/rgbled_utils.hpp"

#include "user_credential_manager/user_credential_manager.hpp"

#include "network_status_led.hpp"

class Model
{
    
    public:

        void Startup();

    private:

        NetworkStatusLed::NetworkStatusLed * m_Led;
        NetworkStatusLed::NetworkLedEventListener * m_WifiListenerLed;
        WifiExtender::WifiExtenderIf * pWifiExtender;
        WifiExtender::WifiExtenderScannerIf * pWifiScannerIf;
        UserCredential::UserCredentialManager *pUserCredentialManager;

        static constexpr uint32_t WIFI_EXTENDER_QUEUE_SIZE = 16;
        QueueHandle_t m_WifiExtenderEventQueue;
        class DispatchEventListener:
            public WifiExtender::EventListener
        {
            public:

                DispatchEventListener(QueueHandle_t q):
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