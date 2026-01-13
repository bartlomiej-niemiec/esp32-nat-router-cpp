#include "network_status_led.hpp"
#include "rgbled_if/rgbled_factory.hpp"
#include <assert.h>
#include <type_traits>

namespace NetworkStatusLed
{

NetworkStatusLed::NetworkStatusLed(uint32_t gpio_pin_num)
{
    auto rgbledfactory = RgbLed::RgbLedFactory::GetInstance();
    m_Led = rgbledfactory.Create(gpio_pin_num);
    assert(nullptr != m_Led);
}


void NetworkStatusLed::Update(WifiExtender::WifiExtenderState state)
{
    std::visit(
        Visitor2{
            [&](const SolidConfig& cfg) {
                m_Led->Solid(cfg.color);
            },
            [&](const BlinkConfig& cfg) {
                m_Led->Blink(cfg.color, cfg.blink_freq);
            }
        },
        m_LedConfigMatrix[ToIndex(state)]
    );
}


}