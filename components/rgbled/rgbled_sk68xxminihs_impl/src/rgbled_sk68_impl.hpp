#pragma once

#include "rgbled_if/rgbled_if.hpp"
#include "esp_timer.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace RgbLed
{

class Sk68xxminiHsImpl:
    public RgbLedIf
{
    public:
        Sk68xxminiHsImpl(const uint32_t gpio_pin_num);

        void Solid(const RgbColor);

        void Blink(const RgbColor, const uint32_t frequency_hz);

    private:

        static const rmt_symbol_word_t m_reset_sym;

        rmt_channel_handle_t m_txChannel;

        rmt_encoder_handle_t m_encoderHandle;

        rmt_encoder_handle_t m_resetEncoderHandle;

        bool m_BlinkState;

        RgbColor m_BlinkyColor;

        esp_timer_handle_t m_BlinkyTimer;

        SemaphoreHandle_t m_Semaphore;

        static void BlinkTimerCallback(void * pArg);

        void SetColor(const RgbColor & color);

        void StopBlinkyTimer();

        static constexpr RgbColor offColor = {0, 0, 0};
};

}
