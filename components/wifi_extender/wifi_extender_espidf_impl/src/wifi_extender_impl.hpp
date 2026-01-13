#pragma once

#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_config.hpp"
#include "wifi_manager.hpp"
#include "freertos/semphr.h"

namespace WifiExtender
{

class WifiExtenderImpl:
    public WifiExtenderIf
{

public:

    WifiExtenderImpl();

    ~WifiExtenderImpl();

    bool Init();

    bool Startup(const WifiExtenderConfig & config);

    bool Shutdown();

    bool UpdateConfig(const WifiExtenderConfig & config);

    bool RegisterListener(EventListener * pEventListener);

    bool TryToReconnect();

    int GetNoClients() const;

    WifiExtenderState GetState() const;

    WifiExtenderScannerIf * GetScanner()
    {
        return &m_WifiManager;
    }

private:

    WifiExtenderConfig m_CurrentConfig;
    WifiManager m_WifiManager;
    SemaphoreHandle_t m_Semaphore;
};


}
