#include "factory_reset_pb/factory_reset_pb.hpp"
#include "esp_timer.h"
#include <cstddef>

namespace FactoryReset
{

FactoryResetPb::FactoryResetPb(const uint32_t gpio_pin_num, StatusLed::StatusLedIf * pStatusLedIf, WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterIf):
    m_FactoryResetPbGpioPin(static_cast<gpio_num_t>(gpio_pin_num)),
    m_pStatusLedIf(pStatusLedIf),
    m_pWifiNatRouterIf(pWifiNatRouterIf),
    m_FactoryResetStatus()
{
    assert(nullptr != m_pStatusLedIf);
    assert(nullptr != m_pWifiNatRouterIf);

    m_FactoryResetStatus.m_FactoryButtonPressed = 0;
    m_FactoryResetStatus.m_TimeOnPress = 0;
    m_FactoryResetStatus.m_FactoryResetProcessState = FactoryResetProcessState::WAIT;

    assert(true == GPIO_IS_VALID_GPIO(m_FactoryResetPbGpioPin));
    gpio_config_t factoryResetConfig = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&factoryResetConfig));

    ESP_ERROR_CHECK(gpio_intr_enable(m_FactoryResetPbGpioPin));

    ESP_ERROR_CHECK(gpio_isr_handler_add(m_FactoryResetPbGpioPin, ISR_HANDLER, &(m_FactoryResetStatus)));
}

void FactoryResetPb::ISR_HANDLER(void *arg)
{
    FactoryResetPb::FactoryResetStatus * pStatus = reinterpret_cast< FactoryResetPb::FactoryResetStatus *>(arg);
    pStatus->m_FactoryButtonPressed.fetch_xor(1U);
    if (pStatus->m_FactoryButtonPressed == 1U)
    {
        pStatus->m_TimeOnPress = esp_timer_get_time();
    }
    else
    {
        if (pStatus->m_FactoryResetProcessState == FactoryResetProcessState::DONE)
        {
            pStatus->m_FactoryResetProcessState = FactoryResetProcessState::WAIT;
        }
    }
}

void FactoryResetPb::MainLoop()
{
    if (m_FactoryResetStatus.m_FactoryButtonPressed == 1U){

        uint64_t elapsedTimeInS = (m_FactoryResetStatus.m_TimeOnPress - esp_timer_get_time()) / 1e6;

        switch (m_FactoryResetStatus.m_FactoryResetProcessState)
        {
            case FactoryResetProcessState::WAIT:
            {
                if (elapsedTimeInS > 0)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::START;
                    m_pStatusLedIf->Update(status);
                    m_FactoryResetStatus.m_FactoryResetProcessState = FactoryResetProcessState::PRESSED_FOR_1SEC;
                }
            }   
            break;

            case FactoryResetProcessState::PRESSED_FOR_1SEC:
            {
                if (elapsedTimeInS > 8)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::DONE;
                    m_pStatusLedIf->Update(status);

                    WifiNatRouterApp::Command cmd;
                    cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdFactoryReset;
                    m_pWifiNatRouterIf->SendCommand(cmd);

                    m_FactoryResetStatus.m_FactoryResetProcessState = FactoryResetProcessState::DONE;
                }
            }
            break;

            case FactoryResetProcessState::DONE:
            {
                // Don't do anythong just wait for button release
            }
            break;
        }
    }
    else
    {
        if (m_FactoryResetStatus.m_FactoryResetProcessState == FactoryResetProcessState::PRESSED_FOR_1SEC)
        {
            StatusLed::Status status;
            status.type = StatusLed::StatusType::FACTORY_RESET;
            status.factoryResetState = StatusLed::FactoryResetState::CANCEL;
            m_pStatusLedIf->Update(status);
            m_FactoryResetStatus.m_FactoryResetProcessState = FactoryResetProcessState::WAIT;
        }
    }
}


}


