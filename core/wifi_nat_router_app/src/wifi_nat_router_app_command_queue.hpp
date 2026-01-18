#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdint.h>
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"

namespace WifiNatRouterApp
{

class WifiNatRouterAppCommandQueue {

    public:

        enum class WifiNatRouterCmd : uint8_t {
            CmdStartScan,
            CmdSetStaConfig,
            CmdSetApConfig,
            CmdApplyNetConfig,
            CmdFactoryReset
        };

        struct Message {

            Message(){};

            Message(WifiNatRouterCmd c, WifiNatRouter::AccessPointConfig ap = {}, WifiNatRouter::StaConfig sta = {}):
                cmd(c),
                apConfig(ap),
                staConfig(sta)
            {};

            WifiNatRouterCmd cmd;
            WifiNatRouter::AccessPointConfig apConfig;
            WifiNatRouter::StaConfig staConfig;
        };

        WifiNatRouterAppCommandQueue();

        ~WifiNatRouterAppCommandQueue();

        bool Add(const Message & msg);

        bool Receive(Message & msg);

    private:

        QueueHandle_t m_MessageQueue;
        StaticQueue_t m_QueueStorage;

        static constexpr int MESSAGE_QUEUE_SIZE = 16;

        uint8_t m_MessageQueueBuffer[MESSAGE_QUEUE_SIZE * sizeof(Message)];
};

}