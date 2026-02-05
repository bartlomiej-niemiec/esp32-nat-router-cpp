#include "wifi_nat_router_impl.hpp"
#include "utils/MutexLockGuard.hpp"
#include "nvs_flash.h"

namespace WifiNatRouter
{

WifiNatRouterImpl::WifiNatRouterImpl():
        m_CurrentConfig(),
        m_WifiManager(),
        m_Semaphore()
{
    m_Semaphore = xSemaphoreCreateMutex();
    assert(nullptr != m_Semaphore);
}

WifiNatRouterImpl::~WifiNatRouterImpl()
{
    vSemaphoreDelete(m_Semaphore);
}


bool WifiNatRouterImpl::Init()
{
    return true;
}

bool WifiNatRouterImpl::Startup(const WifiNatRouterConfig & config)
{
    MutexLockGuard lockGuard(m_Semaphore);
    if (m_WifiManager.IsStartupPossible())
    {
        if (m_WifiManager.Startup(config))
        {
            m_CurrentConfig = config;
            return true;
        }
    }
    return false;
}

bool WifiNatRouterImpl::RegisterListener(EventListener * pEventListener)
{
    return m_WifiManager.RegisterListener(pEventListener);
}

bool WifiNatRouterImpl::Shutdown()
{
    MutexLockGuard lockGuard(m_Semaphore);
    if (m_WifiManager.IsShutdownPossible())
    {
        return m_WifiManager.Shutdown();
    }
    return false;
}

bool WifiNatRouterImpl::UpdateConfig(const WifiNatRouterConfig & config)
{
    MutexLockGuard lockGuard(m_Semaphore);
    if (m_WifiManager.IsUpdateConfigPossible() == false)
    {
        return false;
    }

    bool isApNewConfig = config.apConfig != m_CurrentConfig.apConfig;
    bool isStaNewConfig = config.staConfig != m_CurrentConfig.staConfig;
    
    if (isApNewConfig || isStaNewConfig)
    {
        AccessPointConfig newApConfig = isApNewConfig ? config.apConfig : m_CurrentConfig.apConfig;
        StaConfig newStaConfig = isStaNewConfig ? config.staConfig : m_CurrentConfig.staConfig;

        WifiNatRouterConfig newConfig(newApConfig, newStaConfig);

        if (m_WifiManager.UpdateConfig(newConfig, isApNewConfig))
        {
            m_CurrentConfig = newConfig;
            return true;
        }
    } 

    return false;
}

bool WifiNatRouterImpl::TryToReconnect()
{
    MutexLockGuard lockGuard(m_Semaphore);
    if (GetState() == WifiNatRouterState::STA_CANNOT_CONNECT)
    {
        if (m_WifiManager.TryToReconnect())
        {
            return m_WifiManager.Shutdown();
        }
    }
    return false;
}

WifiNatRouterState WifiNatRouterImpl::GetState() const
{
    return m_WifiManager.GetState();
}

int WifiNatRouterImpl::GetNoClients() const {
    return m_WifiManager.GetNoClients();
}

const NatRouterStatistics & WifiNatRouterImpl::GetNetworkStatistics()
{
    return m_WifiManager.GetNetworkStatistics();
}

}
