#include "rgbled_sk68_impl.hpp"
#include "utils/MutexLockGuard.hpp"

namespace RgbLed
{

const rmt_symbol_word_t Sk68xxminiHsImpl::m_reset_sym = {
            .duration0 = 6000,  // 6000 ticków @20 MHz = 300 µs
            .level0 = 0,
            .duration1 = 0,
            .level1 = 0
};

Sk68xxminiHsImpl::Sk68xxminiHsImpl(const uint32_t gpio_pin_num):
    m_txChannel(nullptr),
    m_encoderHandle(nullptr),
    m_resetEncoderHandle(nullptr),
    m_BlinkState(false),
    m_BlinkyColor{},
    m_BlinkyTimer(nullptr),
    m_Semaphore(nullptr)
{
    m_Semaphore = xSemaphoreCreateMutex();
    assert(nullptr != m_Semaphore);

    assert(gpio_pin_num < static_cast<uint32_t>(GPIO_NUM_MAX));
    
    rmt_tx_channel_config_t tx_chan_config = {};
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;   // select source clock
    tx_chan_config.gpio_num = static_cast<gpio_num_t>(gpio_pin_num);         // GPIO number
    tx_chan_config.mem_block_symbols = 64;          // memory block size, 64 * 4 = 256 Bytes
    tx_chan_config.resolution_hz = 20 * 1000 * 1000;// 20 MHz tick resolution
    tx_chan_config.trans_queue_depth = 2;           // set the number of transactions that can pend in the background
    tx_chan_config.flags.invert_out = false;        // do not invert output signal
    tx_chan_config.flags.with_dma = false;         // do not need DMA backend
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &m_txChannel));
    ESP_ERROR_CHECK(rmt_enable(m_txChannel));

    rmt_bytes_encoder_config_t encoder_config = {};

    rmt_symbol_word_t bit0 = {};
    bit0.level0 = 1;
    bit0.duration0 = 8;
    bit0.level1 = 0;
    bit0.duration1 = 17;

    rmt_symbol_word_t bit1 = {};
    bit1.level0 = 1;
    bit1.duration0 = 16;
    bit1.level1 = 0;
    bit1.duration1 = 9;

    encoder_config.bit0 = bit0;
    encoder_config.bit1 = bit1;
    encoder_config.flags.msb_first = 1;

    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoder_config, &m_encoderHandle));

    rmt_copy_encoder_config_t copyConfig = {};

    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copyConfig, &m_resetEncoderHandle));

    esp_timer_create_args_t timerArgs = {
        .callback = BlinkTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "RGB Blinky timer",
        .skip_unhandled_events = false
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &m_BlinkyTimer));

}

void Sk68xxminiHsImpl::SetColor(const RgbColor & color)
{
    uint8_t grb[3] = { color.g, color.r, color.b };

    rmt_transmit_config_t tx_cfg = {};
    tx_cfg.loop_count = 0;                 // bez loopów
    tx_cfg.flags.eot_level = 0;

    ESP_ERROR_CHECK(rmt_transmit(m_txChannel, m_encoderHandle, grb, sizeof(grb), &tx_cfg));

    ESP_ERROR_CHECK(rmt_transmit(m_txChannel, m_resetEncoderHandle, &m_reset_sym, sizeof(m_reset_sym), &tx_cfg));

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(m_txChannel, -1));
}

void Sk68xxminiHsImpl::BlinkTimerCallback(void * pArg)
{
    Sk68xxminiHsImpl * sk68xxminihs = reinterpret_cast<Sk68xxminiHsImpl *>(pArg);
    sk68xxminihs->SetColor(sk68xxminihs->m_BlinkState ? sk68xxminihs->m_BlinkyColor : offColor);
    sk68xxminihs->m_BlinkState = !sk68xxminihs->m_BlinkState;
}

void Sk68xxminiHsImpl::Solid(const RgbColor color)
{
    StopBlinkyTimer();
    {
        MutexLockGuard lock(m_Semaphore);
        SetColor(color);
    }
}

void Sk68xxminiHsImpl::Blink(const RgbColor color, const uint32_t frequency_hz)
{
    StopBlinkyTimer();
    {
        MutexLockGuard lock(m_Semaphore);
        m_BlinkyColor = color;
        m_BlinkState = false;
    }
    const uint64_t periodUs = (1.0 / static_cast<double>(frequency_hz)) * 1e6;
    esp_timer_start_periodic(m_BlinkyTimer, periodUs);
}

void Sk68xxminiHsImpl::StopBlinkyTimer()
{
    if (esp_timer_is_active(m_BlinkyTimer))
    {
        esp_timer_stop(m_BlinkyTimer);
    }
}


}