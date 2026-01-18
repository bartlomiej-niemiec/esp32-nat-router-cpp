#pragma once

#include "wifi_nat_router_app_if.hpp"
#include "wifi_nat_router_if/wifi_nat_router_if.hpp"

namespace WifiNatRouterApp
{

class WifiNatRouterAppImpl;

class WifiNatRouterApp
{

    public:

        WifiNatRouterApp(WifiNatRouter::WifiNatRouterIf & rWifiIf);

        ~WifiNatRouterApp();

        WifiNatRouterAppIf & GetAppIf() const;

    private:

        WifiNatRouterAppImpl * m_pWifiNatRouterAppImpl;

};

}