#pragma once

#include "wifi_nat_router_if/wifi_nat_router_if.hpp"
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "wifi_manager.hpp"
#include "freertos/semphr.h"

namespace WifiNatRouter
{

class WifiNatRouterImpl:
    public WifiNatRouterIf
{

public:

    WifiNatRouterImpl();

    ~WifiNatRouterImpl();

    bool Init();

    bool Startup(const WifiNatRouterConfig & config);

    bool Shutdown();

    bool UpdateConfig(const WifiNatRouterConfig & config);

    bool RegisterListener(EventListener * pEventListener);

    bool TryToReconnect();

    int GetNoClients() const;

    WifiNatRouterState GetState() const;

    WifiScannerIf * GetScanner()
    {
        return &m_WifiManager;
    }

    const NatRouterStatistics & GetNetworkStatistics();

private:

    WifiNatRouterConfig m_CurrentConfig;
    WifiManager m_WifiManager;
    SemaphoreHandle_t m_Semaphore;
};


}
