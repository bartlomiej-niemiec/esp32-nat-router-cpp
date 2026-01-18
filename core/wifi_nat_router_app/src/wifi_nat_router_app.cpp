#include "wifi_nat_router_app.hpp"
#include "wifi_nat_router_app_impl.hpp"

#include "wifi_nat_router_if/wifi_nat_router_factory.hpp"

namespace WifiNatRouterApp
{

WifiNatRouterApp::WifiNatRouterApp(WifiNatRouter::WifiNatRouterIf & rWifiIf):
    m_pWifiNatRouterAppImpl(nullptr)
{
    m_pWifiNatRouterAppImpl = new (std::nothrow) WifiNatRouterAppImpl(rWifiIf);
    assert(nullptr != m_pWifiNatRouterAppImpl);
}

WifiNatRouterApp::~WifiNatRouterApp()
{
    delete m_pWifiNatRouterAppImpl;
}

WifiNatRouterAppIf & WifiNatRouterApp::GetAppIf() const
{
    return *m_pWifiNatRouterAppImpl;
}

}