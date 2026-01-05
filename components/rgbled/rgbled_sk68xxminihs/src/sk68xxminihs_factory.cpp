#include "rgbled_if/rgbled_factory.hpp"
#include "rgbled_sk68_impl.hpp"
#include <new>

namespace RgbLed
{

RgbLedFactory::RgbLedFactory(){}

RgbLedFactory & RgbLedFactory::GetInstance()
{
    static RgbLedFactory factory;
    return factory;
}


std::unique_ptr<RgbLedIf> RgbLedFactory::Create(const uint32_t gpio_pin_num)
{
    auto it = m_RgbLedIfMap.find(gpio_pin_num);
    if (it == m_RgbLedIfMap.end())
    {
        m_RgbLedIfMap[gpio_pin_num] = true;
        return std::make_unique<Sk68xxminiHsImpl>(gpio_pin_num);
    }

    return nullptr;
}


}
