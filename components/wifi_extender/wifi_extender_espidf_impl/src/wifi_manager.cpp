#include "wifi_manager.hpp"
#include "wifi_extender_if/wifi_extender_config.hpp"
#include "esp_wifi.h"
#include "esp_log.h"

namespace WifiExtender
{

bool WifiManager::Init()
{
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE("WifiManager", "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE("WifiManager", "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    m_WifiManagerContext.m_WifiAp.Init();
    m_WifiManagerContext.m_WifiAp.SetState(WifiAp::State::INITIALIZED);

    m_WifiManagerContext.m_WifiSta.Init();
    m_WifiManagerContext.m_WifiSta.SetState(WifiSta::State::INITIALIZED);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE)
    {
        ESP_LOGE("WifiManager", "esp_wifi_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_APSTA);

    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE)
    {
        ESP_LOGE("WifiManager", "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_ip_event_handler,
                                                            this,
                                                            &m_wifiAnyInst));
    ESP_LOGI("WifiManager", "Registering WIFIEVENT handler");

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_ip_event_handler,
                                                            this,
                                                            &m_ipAnyInst));
    ESP_LOGI("WifiManager", "Registering IPEVENT handler");

    return true;
}

void WifiManager::WifiManagerMain(void *pArg)
{
    assert(nullptr != pArg);
    WifiManager * pWifiManager = reinterpret_cast<WifiManager *>(pArg);
    MessageQueue::Message recvMsg;
    while (true)
    {
        if (pWifiManager->m_MessageQueue.Receive(recvMsg))
        {
            pWifiManager->OnMessage(recvMsg);
        }
    } 

}

void WifiManager::OnMessage(const MessageQueue::Message & msg)
{
    switch (msg.event) {
      case MessageQueue::EventType::EspWifiEvent: {
        switch (msg.espEventCode) {
          case WIFI_EVENT_AP_START:
            m_WifiManagerContext.OnApStart();
            break;
          case WIFI_EVENT_AP_STOP:
            m_WifiManagerContext.OnApStop();
            break;
          case WIFI_EVENT_AP_STACONNECTED:
            m_WifiManagerContext.OnApStaConnected();
            break;
          case WIFI_EVENT_AP_STADISCONNECTED:
            m_WifiManagerContext.OnApStaDisconnected();
            break;
          case WIFI_EVENT_STA_START:
            m_WifiManagerContext.OnStaStart();
            break;
          case WIFI_EVENT_STA_CONNECTED:
            m_WifiManagerContext.OnStaConnected();
            break;
          case WIFI_EVENT_STA_DISCONNECTED:
            m_WifiManagerContext.OnStaDisconnected();
            break;
          case WIFI_EVENT_STA_STOP:
            m_WifiManagerContext.OnStaStop();
            break;
        }
      }
      break;
      case MessageQueue::EventType::EspIpEvent: {
        if (msg.espEventCode == IP_EVENT_STA_GOT_IP) 
        {
            m_WifiManagerContext.OnStaGotIp();
        }
        else if (msg.espEventCode == IP_EVENT_STA_LOST_IP)
        {
            m_WifiManagerContext.OnStaLostIp();
        }
      }
      break;

      case MessageQueue::EventType::StaTimerReconnect:
        break;
      default:
      break;
    }

    ESP_LOGI("WifiManager", "MsgEvent %s", MessageQueue::GetStringOfEventType(msg.event).data());

    Snapshot s = makeSnapshot();
    printSnapshot(s);
    
    auto dec = reduce(s, msg);
    if (dec.newState)
    {
        m_WifiManagerContext.m_WifiManagerState = dec.next;
        ESP_LOGI("WifiManager", "Changing to state %s", WifiExtender::WifiExtenderHelpers::WifiExtenderStaToString(m_WifiManagerContext.m_WifiManagerState).data());
    }
    
    for (uint8_t i = 0; i < dec.count; ++i) {
        runEffect(dec.effects[i], msg);
    }

    /// Check if new configuration is pending and start up extender with pending config
    MaybeFinalize(makeSnapshot());
}

void WifiManager::MaybeFinalize(const Snapshot& s)
{
    /// Check if new configuration is pending and start up extender with pending config
    if ((s.mgrState == WifiExtenderState::STOPPING || s.mgrState == WifiExtenderState::NEW_CONFIGURATION_PENDING) &&
        s.apState == WifiAp::State::STOPPED &&
        s.staState == WifiSta::State::STOPPED)
    {
        MessageQueue::Message msg(MessageQueue::EventType::InternalStop, -1);
        m_MessageQueue.Add(msg);

        if (s.updateConfig)
        {
            m_StartUpInProgress = true;
            m_WifiManagerContext.m_PendingNewConfiguration = false;
            MessageQueue::Message msg(MessageQueue::EventType::StartReq, -1);
            m_MessageQueue.Add(msg);
        }
    }
}

WifiManager::WifiManager():
    m_WifiScannerStateListeners{},
    m_WifiExtenderStateListeners{},
    m_StartUpInProgress(false),
    m_ScanningActive(false),
    m_ShutdownInProgress(false),
    m_ReconnectCounterVal(0),
    m_StaConnectionTimer(nullptr)
{
    esp_timer_create_args_t timerStaArgs;
    timerStaArgs.callback = RetryConnectToNetwork;
    timerStaArgs.arg = this;
    timerStaArgs.dispatch_method = ESP_TIMER_TASK;
    timerStaArgs.name = "STA_CONNECTION_TIMER";
    timerStaArgs.skip_unhandled_events = true;

    ESP_ERROR_CHECK(esp_timer_create(&timerStaArgs, &m_StaConnectionTimer));

    assert(true == Init());

    xTaskCreate(
        WifiManagerMain,
        m_pTaskName,
        TASK_STACK_SIZE,
        this,
        TASK_PRIO,
        &m_TaskHandle
    );
    assert(nullptr != m_TaskHandle);
    ESP_LOGI("WifiManager", "Init Done");
}

WifiManager::~WifiManager()
{
    esp_timer_delete(m_StaConnectionTimer);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_wifiAnyInst);
    esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, m_ipAnyInst);
    vTaskDelete(m_TaskHandle);
    esp_netif_deinit();
}

bool WifiManager::Startup(const WifiExtenderConfig & config)
{
    MessageQueue::Message msg(MessageQueue::EventType::StartReq, -1);
    m_StartUpInProgress = true;
    m_PendingConfig = config;
    bool res = m_MessageQueue.Add(msg);
    return res;
}

bool WifiManager::Shutdown()
{
    MessageQueue::Message msg(MessageQueue::EventType::StopReq, -1);
    m_ShutdownInProgress = true;
    m_StartUpInProgress = false;
    m_ScanningActive = false;
    m_WifiManagerContext.m_PendingNewConfiguration = false;
    bool res = m_MessageQueue.Add(msg);
    return res;
}

bool WifiManager::UpdateConfig(const WifiExtenderConfig & config)
{
    MessageQueue::Message msg(MessageQueue::EventType::UpdateConfigReq, -1);
    m_WifiManagerContext.m_PendingNewConfiguration = true;
    m_PendingConfig = config;
    bool res = m_MessageQueue.Add(msg);
    return res;
}

bool WifiManager::TryToReconnect()
{   
    if (m_ReconnectCounterVal == RECONNECT_COUNTER_ATTEMPTS_COUNT)
    {
        m_ReconnectCounterVal = 0;
        MessageQueue::Message msg(MessageQueue::EventType::StaTimerReconnect, -1);
        return m_MessageQueue.Add(msg);
    }
    return false;
}

bool WifiManager::RegisterListener(EventListener * pEventListener)
{
    assert(pEventListener != nullptr);
    m_WifiExtenderStateListeners.push_back(pEventListener);
    return true;
}

void WifiManager::Stop()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void WifiManager::wifi_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

    WifiManager* pWifiManager = reinterpret_cast<WifiManager*>(arg);

    MessageQueue::Message msg = {};

    bool interesting = false;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
            case WIFI_EVENT_AP_STOP:
            case WIFI_EVENT_AP_STACONNECTED:
            case WIFI_EVENT_AP_STADISCONNECTED:
            case WIFI_EVENT_STA_CONNECTED:
            case WIFI_EVENT_STA_START:
            case WIFI_EVENT_STA_STOP:
            case WIFI_EVENT_STA_DISCONNECTED:
            case WIFI_EVENT_SCAN_DONE:
            {
                msg.event = MessageQueue::EventType::EspWifiEvent;
                msg.espEventCode = event_id;
                interesting = true; 
            }
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
            case IP_EVENT_STA_LOST_IP:
            {
                msg.event = MessageQueue::EventType::EspIpEvent;
                msg.espEventCode = event_id;
                interesting = true;
            }
            break;
            
        };
    }

    if (interesting)
    {
       pWifiManager->m_MessageQueue.Add(msg);
    }
}

void WifiManager::RetryConnectToNetwork(void *arg)
{
    WifiManager* pWifiManager = reinterpret_cast<WifiManager*>(arg);
    pWifiManager->m_ReconnectCounterVal++;
    ESP_LOGI("WifiExtender", "Timer expired attempt: %i", pWifiManager->m_ReconnectCounterVal);
    MessageQueue::Message msg(MessageQueue::EventType::StaTimerReconnect, -1);
    pWifiManager->m_MessageQueue.Add(msg);
}

void WifiManager::StartStaBackoffTimer()
{
    if (esp_timer_is_active(m_StaConnectionTimer)) {
        ESP_ERROR_CHECK(esp_timer_stop(m_StaConnectionTimer));
    }
    ESP_ERROR_CHECK(esp_timer_start_once(m_StaConnectionTimer, TIMER_EXPIRED_TIME_US));
}

void WifiManager::StopStaBackoffTimer()
{
    if (esp_timer_is_active(m_StaConnectionTimer))
    {
        ESP_ERROR_CHECK(esp_timer_stop(m_StaConnectionTimer));
    }
    m_ReconnectCounterVal = 0;
}

WifiExtenderState WifiManager::GetState() const
{
    return m_WifiManagerContext.m_WifiManagerState;
}

WifiManager::Snapshot WifiManager::makeSnapshot() const
{
    return {
        .mgrState = m_WifiManagerContext.m_WifiManagerState,
        .apState = m_WifiManagerContext.m_WifiAp.GetState(),
        .staState = m_WifiManagerContext.m_WifiSta.GetState(),
        .scannerState = m_WifiScanner.GetScannerState(),
        .scanningActive = m_ScanningActive,
        .updateConfig = m_WifiManagerContext.m_PendingNewConfiguration,
        .startUpInProgress = m_StartUpInProgress,
        .staCfgValid = m_WifiManagerContext.m_StaConfigurationValid,
        .reconnectCounterVal = m_ReconnectCounterVal
    };
}

void WifiManager::printSnapshot(const WifiManager::Snapshot& s)
{
    ESP_LOGI("WifiExtender", "Snapshot:");
    ESP_LOGI("WifiExtender", "WifiManagerState=%s", WifiExtender::WifiExtenderHelpers::WifiExtenderStaToString(s.mgrState).data());
    ESP_LOGI("WifiExtender", "ApState=%i", static_cast<int>(s.apState));
    ESP_LOGI("WifiExtender", "staState=%i", static_cast<int>(s.staState));
    ESP_LOGI("WifiExtender", "scannerState=%s", getScannerStateString(s.scannerState).data());
    ESP_LOGI("WifiExtender", "scanningActive=%s", s.scanningActive ? "True" : "False");
    ESP_LOGI("WifiExtender", "updateConfig=%s", s.updateConfig ? "True" : "False");
    ESP_LOGI("WifiExtender", "startUpInProgress=%s", s.startUpInProgress ? "True" : "False");
    ESP_LOGI("WifiExtender", "staCfgValid=%s", s.staCfgValid ? "True" : "False");
    ESP_LOGI("WifiExtender", "reconnectCounterVal=%i", s.reconnectCounterVal);
}

WifiManager::Decision WifiManager::reduce(const WifiManager::Snapshot& s, const MessageQueue::Message& msg) const
{
    Decision d = {};
    switch (msg.event)
    {
        case MessageQueue::EventType::StartReq:
        {
            d.next = WifiExtenderState::STARTED;
            d.newState = true;
            push(d, Effect::ApplyConfig);
            push(d, Effect::WifiStart);
            push(d, Effect::NotifyListener);
        }
        break;

        case MessageQueue::EventType::StopReq:
        {
            d.next = WifiExtenderState::STOPPING;
            d.newState = true;
            push(d, Effect::DisableNat);
            push(d, Effect::WifiStop);
            push(d, Effect::NotifyListener);
        }
        break;

        case MessageQueue::EventType::UpdateConfigReq:
        {
            d.next = WifiExtenderState::NEW_CONFIGURATION_PENDING;
            d.newState = true;
            push(d, Effect::DisableNat);
            push(d, Effect::WifiStop);
            push(d, Effect::NotifyListener);
        }
        break;

        case MessageQueue::EventType::ScanStartReq:
        {
            if (s.mgrState == WifiExtenderState::STA_CANNOT_CONNECT)
            {
                push(d, Effect::StaDisconnect);
                push(d, Effect::StopStaBackoffTimer);
            }
            push(d, Effect::ClearLastScanResults);
            push(d, Effect::StartScan);
            push(d, Effect::NotifyScannerListener);
        }
        break;

        case MessageQueue::EventType::CancelScanReq:
        {
            push(d, Effect::CancelScan);
            push(d, Effect::NotifyScannerListener);
        }
        break;

        case MessageQueue::EventType::EspWifiEvent:
        {
            switch (msg.espEventCode) {
                case WIFI_EVENT_AP_START:
                    if (s.staCfgValid == false)
                    {
                        d.next = WifiExtenderState::RUNNING;
                        d.newState = true;
                        push(d, Effect::NotifyListener);
                    }
                    break;
                case WIFI_EVENT_AP_STADISCONNECTED:
                    if (s.staState == WifiSta::State::CONNECTED)
                    {
                        push(d, Effect::StartStaBackoffTimer);
                        push(d, Effect::StaConnect);
                    }
                    break;  
                case WIFI_EVENT_STA_START:
                    if (s.staCfgValid && s.mgrState !=  WifiExtenderState::STA_CANNOT_CONNECT)
                    {
                        d.next = WifiExtenderState::CONNECTING;
                        d.newState = true;
                        push(d, Effect::StaConnect);
                        push(d, Effect::StartStaBackoffTimer);
                        push(d, Effect::NotifyListener);
                    }
                    break;
                case WIFI_EVENT_SCAN_DONE:
                {
                    if (m_ScanningActive)
                    {
                        push(d, Effect::SignalThatScanCmpl);
                        push(d, Effect::NotifyScannerListener);
                    }
                    else
                    {
                        push(d, Effect::ClearLastScanResults);
                    }

                    if (s.mgrState == WifiExtenderState::STA_CANNOT_CONNECT)
                    {
                        push(d, Effect::StartStaBackoffTimer);
                    }
                }
                break;
        }
        }
        break;

        case MessageQueue::EventType::EspIpEvent:
        {
            switch (msg.espEventCode) {
                case IP_EVENT_STA_GOT_IP:
                    d.next = WifiExtenderState::RUNNING;
                    d.newState = true;
                    push(d, Effect::SetUpDns);
                    push(d, Effect::SetDefaultNetIf);
                    push(d, Effect::EnableNat);
                    push(d, Effect::StopStaBackoffTimer);
                    push(d, Effect::NotifyListener);
                    ESP_LOGI("WifiExtender", "Connected to AP");
                    break;
                case IP_EVENT_STA_LOST_IP:
                    if (s.mgrState == WifiExtenderState::RUNNING)
                    {
                        d.next = WifiExtenderState::CONNECTING;
                        d.newState = true;
                        push(d, Effect::DisableNat);
                        push(d, Effect::StartStaBackoffTimer);
                        push(d, Effect::NotifyListener);
                        ESP_LOGI("WifiExtender", "Disconnected from AP");
                    }
                    break;
            }
        }
        break;

        case MessageQueue::EventType::StaTimerReconnect:
        {
            if (!s.updateConfig && !s.scanningActive) {
                if (s.reconnectCounterVal < RECONNECT_COUNTER_ATTEMPTS_COUNT)
                {
                    push(d, Effect::StaConnect);
                    push(d, Effect::StartStaBackoffTimer);
                    ESP_LOGI("WifiManager", "Attempting to connect to STA");
                }
                else if (s.reconnectCounterVal == RECONNECT_COUNTER_ATTEMPTS_COUNT)
                {
                    if (s.mgrState != WifiExtenderState::STA_CANNOT_CONNECT)
                    {
                        d.next = WifiExtenderState::STA_CANNOT_CONNECT;
                        d.newState = true;
                        push(d, Effect::NotifyListener);
                    }
                }
            }
        }
        break;

        case MessageQueue::EventType::InternalStop:
        {
            d.next = WifiExtenderState::STOPPED;
            d.newState = true;
            push(d, Effect::SetFalseShutdownFlag);
            push(d, Effect::NotifyListener);
        }
        default:
            break;
    }

    return d;
}

void WifiManager::runEffect(Effect e, const MessageQueue::Message& m)
{
    switch (e) {
        case Effect::ApplyConfig: {
            m_WifiManagerContext.SetStaConfigurationValid(m_PendingConfig.staConfig.IsValid());
            m_WifiManagerContext.m_WifiAp.SetConfig(m_PendingConfig.apConfig);
            m_WifiManagerContext.m_WifiSta.SetConfig(m_PendingConfig.staConfig);

            m_WifiManagerContext.m_WifiAp.SetState(WifiAp::State::CONFIGURED);
            m_WifiManagerContext.m_WifiSta.SetState(WifiSta::State::CONFIGURED);
            if (m_StartUpInProgress)
            {
                m_StartUpInProgress = false;
            }
            break;
        }
        break;

        case Effect::WifiStart:
        {
            ESP_ERROR_CHECK(esp_wifi_start());
            m_ReconnectCounterVal = 0;
        }
        break;

        case Effect::WifiStop:
        {
            Stop();
        }
        break;

        case Effect::EnableNat:
        {
            m_WifiManagerContext.m_WifiAp.EnableNat();
        }
        break;

        case Effect::DisableNat:
        {
            m_WifiManagerContext.m_WifiAp.DisableNat();
        }
        break;

        case Effect::StaConnect:
        {
            (void)esp_wifi_connect();
        }
        break;

        case Effect::StaDisconnect:
        {
            (void)esp_wifi_disconnect();
        }
        break;

        case Effect::SetDefaultNetIf:
        {
            m_WifiManagerContext.m_WifiSta.SetDefaultNetIf();
        }
        break;

        case Effect::SetUpDns:
        {
            esp_netif_dns_info_t dnsInfo = m_WifiManagerContext.m_WifiSta.GetDnsInfo();
            ESP_LOGI("DNS_INFO", "Primary dns ip: %d.%d.%d.%d", esp_ip4_addr1(&(dnsInfo.ip.u_addr.ip4)),
                                                    esp_ip4_addr2(&(dnsInfo.ip.u_addr.ip4)),
                                                    esp_ip4_addr3(&(dnsInfo.ip.u_addr.ip4)),
                                                    esp_ip4_addr4(&(dnsInfo.ip.u_addr.ip4)));
            esp_netif_ip_info_t ipInfo = m_WifiManagerContext.m_WifiSta.GetIpInfo();

            if (ipInfo.gw.addr == dnsInfo.ip.u_addr.ip4.addr)
            {
                ESP_LOGI("DNS_INFO", "Setting default primary dns to 8.8.8.8");
                dnsInfo.ip.u_addr.ip4.addr = esp_netif_ip4_makeu32(8, 8, 8, 8);
            }
                    
            m_WifiManagerContext.m_WifiAp.SetUpDnsOnDhcpServer(dnsInfo);
        }
        break;

        case Effect::StartStaBackoffTimer:
        {
            StartStaBackoffTimer();
        }
        break;

        case Effect::StopStaBackoffTimer:
        {
            StopStaBackoffTimer();
        }
        break;

        case Effect::StartScan:
        {
            m_WifiScanner.StartScanFor(m_WifiScanningOptions.opts);
        }
        break;

        case Effect::CancelScan:
        {
            m_WifiScanner.CancelScan();
            m_ScanningActive = false;
        }
        break;

        case Effect::SignalThatScanCmpl:
        {
            m_WifiScanner.ScanningCompleteSignal();
            m_ScanningActive = false;
        }
        break;

        case Effect::ClearLastScanResults:
        {
            m_WifiScanner.CleanResults();
        }
        break;

        case Effect::NotifyListener:
        for (auto & listener : m_WifiExtenderStateListeners)
        {
            listener->Callback(GetState());
        }
        break;

        case Effect::NotifyScannerListener:
        {
            for (auto & listener : m_WifiScannerStateListeners)
            {
                listener(m_WifiScanner.GetScannerState());
            }
        }
        break;

        case Effect::SetFalseShutdownFlag:
        {
            if (m_ShutdownInProgress)
            {
                m_ShutdownInProgress = false;
            }
        }
        break;

        default: break;
    }
}

bool WifiManager::Scan(const ScanOptions& opts)
{
    bool res = false;
    if (IsScanningPossible())
    {
        m_WifiScanningOptions.opts = opts;
        m_ScanningActive = true;
        MessageQueue::Message msg(MessageQueue::EventType::ScanStartReq, -1);
        res = m_MessageQueue.Add(msg);
    }
    return res;
}

bool WifiManager::CancelScan()
{
    bool res = false;
    if (m_ScanningActive)
    {
        MessageQueue::Message msg(MessageQueue::EventType::CancelScanReq, -1);
        res = m_MessageQueue.Add(msg);
    }
    return res;
}

ScannerState WifiManager::GetCurrentState()
{
    return m_WifiScanner.GetScannerState();
}

const std::vector<WifiNetwork> & WifiManager::GetResults() const
{
    return m_WifiScanner.GetResults();
}
 
void WifiManager::RegisterStateListener(ScannerStateListener cb)
{
   assert(cb != nullptr);
   m_WifiScannerStateListeners.emplace_back(cb);
}

bool WifiManager::IsShutdownPossible() const
{
    const WifiExtenderState state = GetState();
    return (false == m_ScanningActive && false == m_ShutdownInProgress) &&
        (state == WifiExtenderState::CONNECTING ||
        state == WifiExtenderState::STA_CANNOT_CONNECT ||
        state == WifiExtenderState::RUNNING ||
        state == WifiExtenderState::STARTED);
}

bool WifiManager::IsStartupPossible() const
{
    return (false == m_WifiManagerContext.m_PendingNewConfiguration &&
            false == m_StartUpInProgress &&
            false == m_ScanningActive &&
            false == m_ShutdownInProgress) &&
            (GetState() == WifiExtenderState::STOPPED);
}

bool WifiManager::IsUpdateConfigPossible() const
{
   return  (GetState() >= WifiExtenderState::CONNECTING) &&
            (false == m_WifiManagerContext.m_PendingNewConfiguration &&
            false == m_StartUpInProgress &&
            false == m_ScanningActive && 
            false == m_ShutdownInProgress);
}

bool WifiManager::IsScanningPossible()
{
    const WifiExtenderState state = GetState();
    return (false == m_WifiManagerContext.m_PendingNewConfiguration &&
            false == m_StartUpInProgress && 
            false == m_ShutdownInProgress &&
            false == m_ScanningActive) &&
        (state == WifiExtenderState::STA_CANNOT_CONNECT ||
        state == WifiExtenderState::CONNECTING ||
        state == WifiExtenderState::RUNNING);
}

}