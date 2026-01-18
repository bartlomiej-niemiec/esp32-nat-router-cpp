#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdint.h>
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "wifi_nat_router_app_if.hpp"

namespace WifiNatRouterApp
{

class WifiNatRouterAppCommandQueue {

    public:

        WifiNatRouterAppCommandQueue();

        ~WifiNatRouterAppCommandQueue();

        bool Add(const Command & msg);

        bool Receive(Command & msg);

    private:

        QueueHandle_t m_MessageQueue;
        StaticQueue_t m_QueueStorage;

        static constexpr int MESSAGE_QUEUE_SIZE = 16;

        uint8_t m_MessageQueueBuffer[MESSAGE_QUEUE_SIZE * sizeof(Command)];
};

}