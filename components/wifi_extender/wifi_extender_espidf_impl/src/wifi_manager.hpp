#pragma once

#include "esp_netif.h"
#include "esp_wifi.h"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_scanner_if.hpp"
#include "wifi_scanner.hpp"
#include "wifi_manager_context.hpp"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <utility>
#include <array>

namespace WifiExtender
{

class MessageQueue
{
    public:

        enum class EventType : uint8_t {
                StartReq,
                UpdateConfigReq,
                StopReq,
                ScanStartReq,
                CancelScanReq,
                EspWifiEvent,
                EspIpEvent,
                StaTimerReconnect,
                InternalStop
        };

        static const std::string_view GetStringOfEventType(EventType event)
        {
            switch (event)
            {
                case EventType::StartReq: return "StartReq event";
                case EventType::UpdateConfigReq: return "UpdateConfigReq event";
                case EventType::StopReq: return "StopReq event";
                case EventType::EspWifiEvent: return "EspWifiEvent event";
                case EventType::EspIpEvent: return "EspIpEvent event";
                case EventType::StaTimerReconnect: return "StaTimerReconnect event";
                case EventType::InternalStop: return "InternalStop event";
                case EventType::ScanStartReq: return "ScanStartReq event";
                case EventType::CancelScanReq: return "ScanCancel event";
            }

            return "Unknown event";
        }

        struct Message {
            Message(){};

            Message(EventType e, int32_t code):
                event(e),
                espEventCode(code)
            {};

            EventType event;
            int32_t espEventCode;
        };

        MessageQueue():
            m_MessageQueue(),
            m_QueueStorage(),
            m_MessageQueueBuffer()
        {
            m_MessageQueue = xQueueCreateStatic(MESSAGE_QUEUE_SIZE, sizeof(Message), m_MessageQueueBuffer, &m_QueueStorage);
            assert(nullptr != m_MessageQueue);
        }

        ~MessageQueue()
        {
            vQueueDelete(m_MessageQueue);
        }

        bool Add(const Message & msg)
        {
            return xQueueSend(m_MessageQueue, &msg, 0) == pdTRUE;
        }

        bool Receive(Message & msg)
        {
            return xQueueReceive(
            m_MessageQueue,
            &msg,
            portMAX_DELAY
            ) == pdTRUE;
        }


    private:

        QueueHandle_t m_MessageQueue;
        StaticQueue_t m_QueueStorage;

        static constexpr int MESSAGE_QUEUE_SIZE = 16;

        uint8_t m_MessageQueueBuffer[MESSAGE_QUEUE_SIZE * sizeof(Message)];
};

class WifiManager:
    public WifiExtenderIf,
    public WifiExtenderScannerIf
{
    public:

        WifiManager();

        ~WifiManager();

        bool Startup(const WifiExtenderConfig & config);

        bool RegisterListener(EventListener * pEventListener);

        bool Shutdown();

        bool UpdateConfig(const WifiExtenderConfig & config);

        bool TryToReconnect();
        
        WifiExtenderState GetState() const;

        int GetNoClients() const;

        bool Scan(const ScanOptions& opts = {}) override;

        bool CancelScan() override;

        ScannerState GetCurrentState() override;

        const std::vector<WifiNetwork> & GetResults() const override;    
        
        void RegisterStateListener(ScannerStateListener cb) override;

        WifiExtenderScannerIf * GetScanner()
        {
            return this;
        }

        bool IsShutdownPossible() const;

        bool IsStartupPossible() const;

        bool IsUpdateConfigPossible() const;

    private:

        WifiManager(const WifiManager&) = delete;

        WifiManager& operator=(const WifiManager&) = delete;

        WifiManager(WifiManager&&) = delete;
        
        WifiManager& operator=(WifiManager&&) = delete;

        void Stop();

        bool Init();

        static void wifi_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

        static void WifiManagerMain(void *pArg);

        TaskHandle_t m_TaskHandle;

        MessageQueue m_MessageQueue;

        void OnMessage(const MessageQueue::Message & msg);

        WifiManagerContext m_WifiManagerContext;

        WifiScanner m_WifiScanner;

        bool IsScanningPossible();

        struct WifiScanningConfig
        {
            int time_in_s;
            ScanOptions opts;
        };

        WifiScanningConfig m_WifiScanningOptions;

        WifiExtenderConfig m_PendingConfig;

        esp_event_handler_instance_t m_wifiAnyInst;

        esp_event_handler_instance_t m_ipAnyInst;

        std::vector<ScannerStateListener> m_WifiScannerStateListeners;

        std::vector<EventListener *> m_WifiExtenderStateListeners;

        bool m_StartUpInProgress;

        bool m_ScanningActive;

        bool m_ShutdownInProgress;

        static constexpr uint8_t RECONNECT_COUNTER_ATTEMPTS_COUNT = 3;
        uint8_t m_ReconnectCounterVal;

        struct Snapshot{
            const WifiExtenderState mgrState;
            const WifiAp::State apState;
            const WifiSta::State staState;
            const ScannerState scannerState;
            const bool scanningActive;
            const bool updateConfig;
            const bool startUpInProgress;
            const bool staCfgValid;
            const uint8_t reconnectCounterVal;
        };

        enum class Effect : uint8_t {
            None,
            ApplyConfig,
            WifiStart,
            WifiStop,
            StaDisconnect,
            DisableNat,
            EnableNat,
            StaConnect,
            SetUpDns,
            SetDefaultNetIf,
            StartStaBackoffTimer,
            StopStaBackoffTimer,
            StartScan,
            StopScan,
            CancelScan,
            ClearLastScanResults,
            SignalThatScanCmpl,
            NotifyListener,
            NotifyScannerListener,
            SetFalseShutdownFlag
        };

        struct Decision{
            bool newState;
            static constexpr int MAX_EFFECTS_PER_DECISION = 6;
            WifiExtenderState next;
            Effect effects[MAX_EFFECTS_PER_DECISION]{};
            uint8_t count{0};
        };

        static inline void push(Decision& d, Effect e) {
            assert(d.count < Decision::MAX_EFFECTS_PER_DECISION);
            if (d.count < Decision::MAX_EFFECTS_PER_DECISION) {
                d.effects[d.count++] = e;
            }
        }

        Snapshot makeSnapshot() const;

        void printSnapshot(const Snapshot& s);

        Decision reduce(const Snapshot& s, const MessageQueue::Message& msg) const;

        void runEffect(Effect e, const MessageQueue::Message& m);

        void MaybeFinalize(const Snapshot& s);

        static constexpr char m_pTaskName[] = "WifiManagerTask";

        static constexpr int TASK_STACK_SIZE = 3096;

        static constexpr int TASK_PRIO = 10;

        static constexpr uint64_t TIMER_EXPIRED_TIME_US = 30000000; //30 s

        void StartStaBackoffTimer();

        void StopStaBackoffTimer();

        static void RetryConnectToNetwork(void *arg);

        esp_timer_handle_t m_StaConnectionTimer;

};


}
