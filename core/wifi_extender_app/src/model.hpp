#pragma once

#include "freertos/FreeRTOS.h"

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

};