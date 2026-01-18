#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdint.h>
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"

namespace WifiNatRouterApp
{

class WifiNatRouterAppEventQueue {

    public:

        enum class WifiNatRouterEvent : uint8_t {
            RouterState,
            InternetStatus,
            WifiNetworkScanDone
        };

        struct Message {

            Message(){};

            Message(WifiNatRouterEvent e, WifiNatRouter::WifiNatRouterState state, bool ia):
                event(e),
                newState(state),
                InternetAccess(ia)
            {};

            WifiNatRouterEvent event;
            WifiNatRouter::WifiNatRouterState newState;
            bool InternetAccess;
        };

        WifiNatRouterAppEventQueue();

        ~WifiNatRouterAppEventQueue();

        bool Add(const Message & msg);

        bool Receive(Message & msg);

    private:

        QueueHandle_t m_MessageQueue;
        StaticQueue_t m_QueueStorage;

        static constexpr int MESSAGE_QUEUE_SIZE = 16;

        uint8_t m_MessageQueueBuffer[MESSAGE_QUEUE_SIZE * sizeof(Message)];
};

}