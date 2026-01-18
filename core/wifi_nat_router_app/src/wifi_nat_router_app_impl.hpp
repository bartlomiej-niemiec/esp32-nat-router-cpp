#pragma once

#include "wifi_nat_router_app_if.hpp"
#include "wifi_nat_router_if/wifi_nat_router_if.hpp"

#include "wifi_nat_router_app_event_queue.hpp"
#include "wifi_nat_router_app_command_queue.hpp"

#include "network_status_led.hpp"
#include "network_config_manager.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string_view>

namespace WifiNatRouterApp
{

class WifiNatRouterAppImpl:
    public WifiNatRouterAppIf
{

    public:

        WifiNatRouterAppImpl();

        ~WifiNatRouterAppImpl();

        WifiNatRouterAppImpl(
            WifiNatRouter::WifiNatRouterIf & rWifiIf
        );

        bool SendCommand(const Command & cmd) const{
            return false;
        };

        bool TryGetSnapshot(AppSnapshot& out) const{
            return false;
        };

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
                };

            private:
                
                WifiNatRouterAppEventQueue * m_pEventQueue;
        };

        NetworkConfigManager m_NetworkConfigManager;

        WifiNatRouter::WifiNatRouterIf & m_rWifiNatRouter;

        std::unique_ptr<NetworkStatusLed::NetworkStatusLed> m_pLed;

        WifiNatRouterAppEventQueue m_EventQueue;
        WifiNatRouterAppCommandQueue m_CommandQueue;
        WifiEventDispatcher m_EventDispatcher;
        
        static constexpr std::string_view TASK_NAME = "WIFI_NAT_ROUTER_APP";
        static constexpr uint32_t TASK_STACK_SIZE = 8096;
        static constexpr int TASK_PRIORITY = 4;
        TaskHandle_t m_MainTask;
        
        static void MainLoop(void *pArg);

        void ProcessEventQueue();

        void ProcessCommandQueue();

};

}