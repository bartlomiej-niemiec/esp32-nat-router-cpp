#pragma once

#include "rgbled_if.hpp"
#include <unordered_map>
#include <memory>

namespace RgbLed
{

class RgbLedFactory {
    public:

        static RgbLedFactory & GetInstance();

        std::unique_ptr<RgbLedIf> Create(const uint32_t gpio_pin_num);

    private:

        RgbLedFactory();

        std::unordered_map<uint32_t, bool> m_RgbLedIfMap;

};

}