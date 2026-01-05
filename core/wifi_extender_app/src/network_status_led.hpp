#pragma once

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "rgbled_if/rgbled_if.hpp"
#include "rgbled_if/rgbled_utils.hpp"
#include <array>
#include <memory>
#include <variant>

namespace NetworkStatusLed
{

class NetworkStatusLed
{

    public:

        NetworkStatusLed(uint32_t gpio_pin_num);

        void Update(WifiExtender::WifiExtenderState state);
    
    private:

        std::unique_ptr<RgbLed::RgbLedIf> m_Led;

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

        const std::array<configVars, static_cast<size_t>(WifiExtender::WifiExtenderState::STATE_COUNT)> m_LedConfigMatrix = {{
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Red))                  },  // STOPPED
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Blue))                 },  // STARTED
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Green), BLINK_1HZ)     },  // CONNECTING
            { SolidConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Green))                },  // RUNNING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Red), BLINK_1HZ)       }, // STOPPING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Blue), BLINK_1HZ)      }, // NEW_CONFIGURATION_PENDING
            { BlinkConfig(RgbLed::RgbColorCreator::Create(RgbLed::Color::Yellow), BLINK_1HZ)    }  // STA_CANNOT_CONNECT
        }};

        constexpr size_t ToIndex(WifiExtender::WifiExtenderState state)
        {
            return static_cast<size_t>(state);
        }

        template<typename A, typename B>
        struct Visitor2 : A, B {
            Visitor2(A a, B b) : A(a), B(b) {}
            using A::operator();
            using B::operator();
        };

};

class NetworkLedEventListener:
    public WifiExtender::EventListener
{
    public:
        explicit NetworkLedEventListener(NetworkStatusLed * led):
            m_Led(led) {};

        void Callback(WifiExtender::WifiExtenderState event) override
        {
            m_Led->Update(event);
        };

    private:
        NetworkStatusLed * m_Led;
};

}