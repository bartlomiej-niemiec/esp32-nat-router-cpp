#pragma once

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "wifi_nat_router_if/wifi_nat_router_if.hpp"
#include "rgbled_if/rgbled_if.hpp"
#include "rgbled_if/rgbled_utils.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <array>
#include <memory>
#include <variant>

namespace StatusLed
{

enum class StatusType {
    NETWORK_STATUS_UPDATE,
    INTERNET_ACCESS,
    FACTORY_RESET
};

enum class FactoryResetState {
    START,
    CANCEL,
    DONE
};

struct Status {
    StatusType type;
    WifiNatRouter::WifiNatRouterState routerState;
    bool internetAvailable;
    FactoryResetState factoryResetState;
};

class StatusLed
{
    public:

        StatusLed(const uint32_t gpio_pin_num);

        ~StatusLed();

        bool Update (const Status & status);

    private:

        std::unique_ptr<RgbLed::RgbLedIf> m_pStatusLed;

        enum class StatusLedState {
            NETWORK_STATUS,
            FACTORY_RESET_PENDING
        };

        WifiNatRouter::WifiNatRouterState m_CachedRouterState;
        bool m_CachedInternetAccess;
        StatusLedState m_State;

        static constexpr std::string_view TASK_NAME = "RGB_STATUS_LED";
        static constexpr uint32_t TASK_STACK_SIZE = 4096;
        static constexpr int TASK_PRIORITY = 2;

        TaskHandle_t m_MainTask;
        QueueHandle_t m_MessageQueue;
        StaticQueue_t m_QueueStorage;

        static constexpr int MESSAGE_QUEUE_SIZE = 8;
        uint8_t m_MessageQueueBuffer[MESSAGE_QUEUE_SIZE * sizeof(Status)];

        enum class LedState : uint8_t {
            OFF,
            SOLID,
            BLINK
        };
        
        struct SolidConfig {
            RgbLed::RgbColor color;
        };

        struct BlinkConfig {
            RgbLed::RgbColor color;
            uint32_t blink_freq;
        };

        using configVars = std::variant<SolidConfig, BlinkConfig>;

        static constexpr uint32_t BLINK_1HZ = 1;

        const std::array<configVars, static_cast<size_t>(WifiNatRouter::
WifiNatRouterState::STATE_COUNT)> m_RouterStateLedMatrix = {{
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Red))                  },  // STOPPED
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Blue))                 },  // STARTED
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Yellow), BLINK_1HZ)    },  // CONNECTING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Green), BLINK_1HZ)     },  // RUNNING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Red), BLINK_1HZ)       }, // STOPPING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Blue), BLINK_1HZ)      }, // NEW_CONFIGURATION_PENDING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Yellow), BLINK_1HZ)    }  // STA_CANNOT_CONNECT
        }};

        constexpr size_t ToIndex(WifiNatRouter::
WifiNatRouterState state)
        {
            return static_cast<size_t>(state);
        }

        template<typename A, typename B>
        struct Visitor2 : A, B {
            Visitor2(A a, B b) : A(a), B(b) {}
            using A::operator();
            using B::operator();
        };

        static void MainLoop(void *pArg);

        void UpdateLedRouterState(WifiNatRouter::WifiNatRouterState state);

        void UpdateLedInternetAccess(bool internetAvailable);

        void UpdateLedFactoryReset(FactoryResetState factoryResetState);

};

}