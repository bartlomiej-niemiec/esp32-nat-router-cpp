#pragma once

#include "wifi_nat_router_app_if.hpp"
#include "wifi_nat_router_if/wifi_nat_router_if.hpp"

#include "wifi_nat_router_app_event_queue.hpp"
#include "wifi_nat_router_app_command_queue.hpp"

#include "network_config_manager.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "internet_activity_monitor.hpp"

#include "status_led/status_led.hpp"

#include "esp_timer.h"

#include "utils/MutexLockGuard.hpp"

#include <string_view>
#include <optional>

namespace WifiNatRouterApp
{

class WifiNatRouterAppImpl:
    public WifiNatRouterAppIf
{

    public:

        WifiNatRouterAppImpl();

        ~WifiNatRouterAppImpl();

        WifiNatRouterAppImpl(
            WifiNatRouter::WifiNatRouterIf & rWifiIf,
            StatusLed::StatusLedIf * pStatusLed
        );

        bool SendCommand(const Command & cmd);

        bool TryGetSnapshot(AppSnapshot& out) const;

        const WifiNatRouter::NatRouterStatistics & GetRouterStatistics();

    private:
        
        class WifiEventDispatcher:
                public WifiNatRouter::EventListener
        {
            public:

                WifiEventDispatcher(WifiNatRouterAppEventQueue * pEventQueue):
                    m_pEventQueue(pEventQueue){
                        assert(m_pEventQueue != nullptr);
                    };

                void Callback(WifiNatRouter::WifiNatRouterState event) override
                {
                    m_pEventQueue->Add(
                        WifiNatRouterAppEventQueue::Message(
                            WifiNatRouterAppEventQueue::WifiNatRouterEvent::RouterState,
                            event,
                            false
                        )
                    );
                    ESP_LOGI("EventQueue", "Adding new event");
                };

            private:
                
                WifiNatRouterAppEventQueue * m_pEventQueue;
        };

        NetworkConfigManager m_NetworkConfigManager;

        WifiNatRouter::WifiNatRouterIf & m_rWifiNatRouter;
        StatusLed::StatusLedIf  * m_pStatusLed;

        WifiNatRouterAppEventQueue m_EventQueue;
        WifiNatRouterAppCommandQueue m_CommandQueue;
        WifiEventDispatcher m_EventDispatcher;

        InternetActivityMonitor m_InternetActivityMonitor;
        esp_timer_handle_t m_InternetActivityTimer;
        static void InternetActivityTimerCb(void * pArgs);

        static constexpr std::string_view TASK_NAME = "WIFI_NAT_ROUTER_APP";
        static constexpr uint32_t TASK_STACK_SIZE = 8096;
        static constexpr int TASK_PRIORITY = 4;
        TaskHandle_t m_MainTask;

        AppSnapshot m_WorkingAppSnapshot;
        AppSnapshot m_ApiSnapShot;
        WifiNatRouter::WifiNatRouterConfig m_PendingConfig;

        mutable SemaphoreHandle_t m_SnapshotMutex;

        static void MainLoop(void *pArg);

        void ProcessEventQueue();

        void ProcessCommandQueue();

        void CommitSnapshot();
};

}