#pragma once

#include <stdint.h>
#include <string>
#include "wifi_nat_router_config.hpp"
#include "wifi_scanner_if.hpp"

namespace WifiNatRouter
{

class EventListener
{
    public:

        virtual ~EventListener() = default;

        virtual void Callback(WifiNatRouterState event) = 0;

};

class WifiNatRouterIf{

public:

    ~WifiNatRouterIf() = default;

    virtual bool Startup(const WifiNatRouterConfig & config) = 0;

    virtual bool RegisterListener(EventListener * pEventListener) = 0;

    virtual WifiNatRouterState GetState() const = 0;

    virtual bool Shutdown() = 0;

    virtual bool TryToReconnect() = 0;

    virtual bool UpdateConfig(const WifiNatRouterConfig & config) = 0;

    virtual WifiScannerIf * GetScanner() = 0;

    virtual int GetNoClients() const = 0;

    virtual const NatRouterStatistics & GetNetworkStatistics() = 0;

};

}