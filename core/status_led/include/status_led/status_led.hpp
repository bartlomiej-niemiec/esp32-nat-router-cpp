#pragma once

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "rgbled_if/rgbled_if.hpp"
#include "rgbled_if/rgbled_utils.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "status_led/status_led_if.hpp"

#include <array>
#include <memory>
#include <variant>
#include <optional>

namespace StatusLed
{


class StatusLed:
    public StatusLedIf
{
    public:

        StatusLed(const uint32_t gpio_pin_num);

        ~StatusLed();

        bool Update (const Status & status);

        void MainLoop();

    private:

        std::unique_ptr<RgbLed::RgbLedIf> m_pStatusLed;

        enum class StatusLedState {
            NETWORK_STATUS,
            FACTORY_RESET_PENDING
        };

        WifiNatRouter::WifiNatRouterState m_CachedRouterState;
        bool m_CachedInternetAccess;
        StatusLedState m_State;

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

        const std::array<std::optional<configVars>, static_cast<size_t>(FactoryResetState::MAX_STATE)> m_FactoryResetLedMatrix = {{
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Purple), BLINK_1HZ)    },  // START
            { std::nullopt                                                                      },  // CANCEL
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Purple))               },  // DONE_WAIT_FOR_RELEASE
            { std::nullopt                                                                      },  // DONE
        }};

        constexpr size_t ToIndex(FactoryResetState state)
        {
            return static_cast<size_t>(state);
        }

        template<typename A, typename B>
        struct Visitor2 : A, B {
            Visitor2(A a, B b) : A(a), B(b) {}
            using A::operator();
            using B::operator();
        };

        
        void UpdateLedRouterState(WifiNatRouter::WifiNatRouterState state);

        void UpdateLedInternetAccess(bool internetAvailable);

        void UpdateLedFactoryReset(FactoryResetState factoryResetState);

};

}