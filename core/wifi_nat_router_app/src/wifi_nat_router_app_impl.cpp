#include "wifi_nat_router_app_impl.hpp"

#include "wifi_nat_router_app_config.hpp"

namespace WifiNatRouterApp
{

WifiNatRouterAppImpl::WifiNatRouterAppImpl(WifiNatRouter::WifiNatRouterIf & rWifiIf):
    m_NetworkConfigManager(),
    m_rWifiNatRouter(rWifiIf),
    m_pLed(nullptr),
    m_EventQueue(),
    m_CommandQueue(),
    m_EventDispatcher(&m_EventQueue),
    m_InternetActivityMonitor(m_EventQueue),
    m_InternetActivityTimer(nullptr),
    m_MainTask(nullptr),
    m_CachedAppSnapshot()
{
    if (ENABLE_RGB_LED)
    {
        m_pLed = std::make_unique<NetworkStatusLed::NetworkStatusLed>(RGB_LED_GPIO_PIN);
    }

    m_rWifiNatRouter.RegisterListener(&m_EventDispatcher);

    esp_timer_create_args_t internetAccessTimerArgs = {
        .callback = InternetActivityTimerCb,
        .arg = &m_InternetActivityMonitor,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "InternetCheckerTimer",
        .skip_unhandled_events = true
    };

    ESP_ERROR_CHECK(esp_timer_create(
        &internetAccessTimerArgs,
        &m_InternetActivityTimer
    )); 

    xTaskCreate(
            MainLoop,
            TASK_NAME.data(),
            TASK_STACK_SIZE,
            this,
            TASK_PRIORITY,
            &m_MainTask
    );
    assert(nullptr != m_MainTask);
}

WifiNatRouterAppImpl::~WifiNatRouterAppImpl()
{
    vTaskDelete(m_MainTask);
}

bool WifiNatRouterAppImpl::SendCommand(const Command & cmd)
{
    return m_CommandQueue.Add(cmd);
}

bool WifiNatRouterAppImpl::TryGetSnapshot(AppSnapshot& out) const
{
    out = m_CachedAppSnapshot;
    return true;
}

void WifiNatRouterAppImpl::InternetActivityTimerCb(void * pArgs)
{
    InternetActivityMonitor * pIaMonitor = reinterpret_cast<InternetActivityMonitor*>(pArgs);
    pIaMonitor->Check();
}

void WifiNatRouterAppImpl::MainLoop(void *pArg)
{
    WifiNatRouterAppImpl * pInstance = reinterpret_cast<WifiNatRouterAppImpl*>(pArg);
    constexpr uint32_t TASK_DELAY_MS = 10;
    constexpr uint32_t TASK_DELAY_TICKS = pdMS_TO_TICKS(TASK_DELAY_MS);
    pInstance->m_rWifiNatRouter.Startup(
        {pInstance->m_NetworkConfigManager.GetApConfig(), pInstance->m_NetworkConfigManager.GetStaConfig()}
    );

    pInstance->m_CachedAppSnapshot.config = {pInstance->m_NetworkConfigManager.GetApConfig(), pInstance->m_NetworkConfigManager.GetStaConfig()};
    pInstance->m_CachedAppSnapshot.routerState = pInstance->m_rWifiNatRouter.GetState();
    pInstance->m_CachedAppSnapshot.scannedNetworks = {};
    pInstance->m_CachedAppSnapshot.scannedCount = 0;
    pInstance->m_CachedAppSnapshot.scanState = pInstance->m_rWifiNatRouter.GetScanner()->GetCurrentState();
    pInstance->m_CachedAppSnapshot.configApplyInProgress = false;
    pInstance->m_CachedAppSnapshot.internetAccess = false;
    pInstance->m_CachedAppSnapshot.noApClients = 0;

    for (;;)
    {
        pInstance->ProcessEventQueue();
        pInstance->ProcessCommandQueue();
        vTaskDelay(TASK_DELAY_TICKS);
    }
}

void WifiNatRouterAppImpl::ProcessEventQueue()
{
    WifiNatRouterAppEventQueue::Message msg;

    if (m_EventQueue.Receive(msg))
    {
        switch (msg.event)
        {
            case WifiNatRouterAppEventQueue::WifiNatRouterEvent::RouterState:
            {
                m_pLed->Update(msg.newState);
                m_CachedAppSnapshot.routerState = msg.newState;
                if (m_NewConfigInProgress && msg.newState == WifiNatRouter::WifiNatRouterState::CONNECTING)
                {
                    m_NewConfigInProgress = false;
                }

                /// TODO - handle it in command queue
                if (msg.newState == WifiNatRouter::WifiNatRouterState::NEW_CONFIGURATION_PENDING)
                {
                    if(esp_timer_is_active(m_InternetActivityTimer))
                    {
                        esp_timer_stop(m_InternetActivityTimer);
                    }
                }
                else if (msg.newState == WifiNatRouter::WifiNatRouterState::RUNNING)
                {
                    m_InternetActivityMonitor.Check();
                    if (!esp_timer_is_active(m_InternetActivityTimer))
                    {
                        ESP_ERROR_CHECK(esp_timer_start_periodic(
                            m_InternetActivityTimer,
                            10000000 //10s
                        ));
                    }
                }
            }
            break;

            case WifiNatRouterAppEventQueue::WifiNatRouterEvent::InternetStatus:
            {
                // TODO Update LED
                m_CachedAppSnapshot.internetAccess = msg.InternetAccess;
            }
            break;

            case WifiNatRouterAppEventQueue::WifiNatRouterEvent::WifiNetworkScanDone:
            {
                const auto networks = m_rWifiNatRouter.GetScanner()->GetResults();
                std::copy(std::begin(networks), std::end(networks), std::begin(m_CachedAppSnapshot.scannedNetworks));
                m_CachedAppSnapshot.scannedCount = std::min(networks.size(), static_cast<size_t>(AppSnapshot::WIFI_NETWORK_SCAN_LIMIT));
                m_CachedAppSnapshot.scanState = WifiNatRouter::ScannerState::Done;
            }
            break;
        }
    }

    m_CachedAppSnapshot.noApClients = m_rWifiNatRouter.GetNoClients();
    m_CachedAppSnapshot.configApplyInProgress = m_NewConfigInProgress;
    m_CachedAppSnapshot.scanState = m_rWifiNatRouter.GetScanner()->GetCurrentState();

}

void WifiNatRouterAppImpl::ProcessCommandQueue()
{
    WifiNatRouterApp::Command cmd;

    if (m_CommandQueue.Receive(cmd))
    {
        switch (cmd.cmd)
        {
            case WifiNatRouterApp::WifiNatRouterCmd::CmdStartScan:
            {
                m_rWifiNatRouter.GetScanner()->Scan();
            }
            break;

            case WifiNatRouterApp::WifiNatRouterCmd::CmdSetStaConfig:
            {
                m_PendingConfig.staConfig = cmd.staConfig;
            }
            break;

            case WifiNatRouterApp::WifiNatRouterCmd::CmdSetApConfig:
            {
                m_PendingConfig.apConfig = cmd.apConfig;
            }
            break;

            case WifiNatRouterApp::WifiNatRouterCmd::CmdApplyNetConfig:
            {
                m_NewConfigInProgress = m_rWifiNatRouter.UpdateConfig(m_PendingConfig);
            }
            break;

            case WifiNatRouterApp::WifiNatRouterCmd::CmdFactoryReset:
            {
                uint32_t ipAddress, netmask;
                WifiNatRouter::WifiNatRouterHelpers::ConvertStringToIpAddress(DEFAULT_AP_IP_ADDR.data(), ipAddress);
                WifiNatRouter::WifiNatRouterHelpers::ConvertStringToIpAddress(DEFAULT_AP_NETMASK.data(), netmask);

                WifiNatRouter::AccessPointConfig defaultApConfig(
                    DEFAULT_AP_SSID,
                    DEFAULT_AP_PASSWORD,
                    ipAddress,
                    netmask
                );

                WifiNatRouter::StaConfig defaultStaConfig(
                    DEFAULT_STA_SSID,
                    DEFAULT_STA_PASSWORD
                );
                
                m_NetworkConfigManager.SetApConfig(defaultApConfig);
                m_NetworkConfigManager.SetStaConfig(defaultStaConfig);

                m_NewConfigInProgress = m_rWifiNatRouter.UpdateConfig({defaultApConfig, defaultStaConfig});
            }
            break;

        }
    }

}

}