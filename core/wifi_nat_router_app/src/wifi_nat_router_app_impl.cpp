#include "wifi_nat_router_app_impl.hpp"

#include "wifi_nat_router_app_config.hpp"

namespace WifiNatRouterApp
{

WifiNatRouterAppImpl::WifiNatRouterAppImpl(WifiNatRouter::WifiNatRouterIf & rWifiIf,  StatusLed::StatusLedIf * pStatusLed):
    m_NetworkConfigManager(),
    m_rWifiNatRouter(rWifiIf),
    m_pStatusLed(pStatusLed),
    m_EventQueue(),
    m_CommandQueue(),
    m_EventDispatcher(&m_EventQueue),
    m_InternetActivityMonitor(m_EventQueue),
    m_InternetActivityTimer(nullptr),
    m_MainTask(nullptr),
    m_WorkingAppSnapshot(),
    m_SnapshotMutex(nullptr)
{
    assert(nullptr != m_pStatusLed);

    m_rWifiNatRouter.RegisterListener(&m_EventDispatcher);

    m_rWifiNatRouter.GetScanner()->RegisterStateListener(
        [this](WifiNatRouter::ScannerState state){
            if (state == WifiNatRouter::ScannerState::Done){
                WifiNatRouterAppEventQueue::Message event;
                event.event = WifiNatRouterAppEventQueue::WifiNatRouterEvent::WifiNetworkScanDone;
                m_EventQueue.Add(event);
            }
        }
    );

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

    m_SnapshotMutex = xSemaphoreCreateMutex();
    assert(m_SnapshotMutex != nullptr);

    m_PendingConfig.apConfig = m_NetworkConfigManager.GetApConfig();
    m_PendingConfig.staConfig = m_NetworkConfigManager.GetStaConfig();

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
    if (m_MainTask)
    {
        vTaskDelete(m_MainTask);
    }
    
    if (m_SnapshotMutex) {
        vSemaphoreDelete(m_SnapshotMutex);
        m_SnapshotMutex = nullptr;
    }
}

bool WifiNatRouterAppImpl::SendCommand(const Command & cmd)
{
    return m_CommandQueue.Add(cmd);
}

bool WifiNatRouterAppImpl::TryGetSnapshot(AppSnapshot& out) const
{
    MutexLockGuard lock(m_SnapshotMutex, 0);
    if (!lock.locked()) return false;
    out = m_ApiSnapShot;
    return true;
}

const WifiNatRouter::NatRouterStatistics & WifiNatRouterAppImpl::GetRouterStatistics()
{
    return m_rWifiNatRouter.GetNetworkStatistics();
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

    pInstance->m_WorkingAppSnapshot.config = {pInstance->m_NetworkConfigManager.GetApConfig(), pInstance->m_NetworkConfigManager.GetStaConfig()};
    pInstance->m_WorkingAppSnapshot.routerState = pInstance->m_rWifiNatRouter.GetState();
    pInstance->m_WorkingAppSnapshot.scannedNetworks = {};
    pInstance->m_WorkingAppSnapshot.scannedCount = 0;
    pInstance->m_WorkingAppSnapshot.scanState = pInstance->m_rWifiNatRouter.GetScanner()->GetCurrentState();
    pInstance->m_WorkingAppSnapshot.configApplyInProgress = false;
    pInstance->m_WorkingAppSnapshot.internetAccess = false;
    pInstance->m_WorkingAppSnapshot.noApClients = 0;
    pInstance->m_ApiSnapShot = pInstance->m_WorkingAppSnapshot;

    for (;;)
    {
        pInstance->ProcessEventQueue();
        pInstance->ProcessCommandQueue();
        pInstance->CommitSnapshot();
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
                if (m_pStatusLed)
                {
                    StatusLed::Status ledStatus;
                    ledStatus.type = StatusLed::StatusType::NETWORK_STATUS_UPDATE;
                    ledStatus.routerState = msg.newState;
                    m_pStatusLed->Update(ledStatus);
                }

                m_WorkingAppSnapshot.routerState = msg.newState;
                if (m_WorkingAppSnapshot.configApplyInProgress && msg.newState == WifiNatRouter::WifiNatRouterState::CONNECTING)
                {
                    m_WorkingAppSnapshot.configApplyInProgress = false;
                    m_WorkingAppSnapshot.readyForApplyingConfig = false;
                }

                if (msg.newState == WifiNatRouter::WifiNatRouterState::RUNNING)
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
                else
                {
                    if(esp_timer_is_active(m_InternetActivityTimer))
                    {
                        esp_timer_stop(m_InternetActivityTimer);
                    }
                }
            }
            break;

            case WifiNatRouterAppEventQueue::WifiNatRouterEvent::InternetStatus:
            {
                m_WorkingAppSnapshot.internetAccess = msg.InternetAccess;
                if (m_pStatusLed)
                {
                    StatusLed::Status ledStatus;
                    ledStatus.type = StatusLed::StatusType::INTERNET_ACCESS;
                    ledStatus.internetAvailable = msg.InternetAccess;
                    m_pStatusLed->Update(ledStatus);
                }
            }
            break;

            case WifiNatRouterAppEventQueue::WifiNatRouterEvent::WifiNetworkScanDone:
            {
                const auto networks = m_rWifiNatRouter.GetScanner()->GetResults();
                m_WorkingAppSnapshot.scannedCount = std::min(networks.size(), static_cast<size_t>(AppSnapshot::WIFI_NETWORK_SCAN_LIMIT));
                std::copy_n(std::begin(networks), m_WorkingAppSnapshot.scannedCount, std::begin(m_WorkingAppSnapshot.scannedNetworks));
            }
            break;
        }
    }

    m_WorkingAppSnapshot.noApClients = m_rWifiNatRouter.GetNoClients();
    m_WorkingAppSnapshot.scanState = m_rWifiNatRouter.GetScanner()->GetCurrentState();
    m_WorkingAppSnapshot.readyForApplyingConfig = m_PendingConfig != m_WorkingAppSnapshot.config;

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
                m_WorkingAppSnapshot.configApplyInProgress = m_rWifiNatRouter.UpdateConfig(m_PendingConfig);
                if (m_WorkingAppSnapshot.configApplyInProgress)
                {
                    m_NetworkConfigManager.SetApConfig(m_PendingConfig.apConfig);
                    m_NetworkConfigManager.SetStaConfig(m_PendingConfig.staConfig);
                    m_WorkingAppSnapshot.config = m_PendingConfig;
                }
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

                m_WorkingAppSnapshot.configApplyInProgress = m_rWifiNatRouter.UpdateConfig({defaultApConfig, defaultStaConfig});
                if (m_WorkingAppSnapshot.configApplyInProgress)
                {
                    m_PendingConfig = {defaultApConfig, defaultStaConfig};
                    m_WorkingAppSnapshot.config = m_PendingConfig;
                    m_WorkingAppSnapshot.readyForApplyingConfig = false;
                }

            }
            break;

        }
    }

}

void WifiNatRouterAppImpl::CommitSnapshot()
{
     MutexLockGuard lock(m_SnapshotMutex, pdMS_TO_TICKS(5));
    if (lock.locked()) {
        m_ApiSnapShot = m_WorkingAppSnapshot;
    }
}

}