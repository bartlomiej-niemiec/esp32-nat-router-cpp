#include "factory_reset_pb/factory_reset_pb.hpp"
#include "esp_timer.h"
#include <cstddef>

namespace FactoryReset
{

FactoryResetPb::FactoryResetPb(const Config & FactoryButtonConfig, StatusLed::StatusLedIf * pStatusLedIf, WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterIf):
    m_FactoryButtonConfig(FactoryButtonConfig),
    m_pStatusLedIf(pStatusLedIf),
    m_pWifiNatRouterIf(pWifiNatRouterIf),
    m_prevLevel(true)
{
    assert(nullptr != m_pWifiNatRouterIf);

    m_TimeOnPress = 0;
    m_FactoryResetProcessState = FactoryResetProcessState::WAIT;

    assert(true == GPIO_IS_VALID_GPIO(m_FactoryButtonConfig.gpio_pin_num));
    
    gpio_config_t factoryResetConfig = {
        .pin_bit_mask = static_cast<uint64_t>(1 << m_FactoryButtonConfig.gpio_pin_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&factoryResetConfig));
}

void FactoryResetPb::MainLoop()
{
    bool level = static_cast<bool>(gpio_get_level(m_FactoryButtonConfig.gpio_pin_num));
    double elapsedTimeInS = 0;


    if (!level && m_prevLevel)
    {
        m_TimeOnPress = esp_timer_get_time();
    }

    if (!level)
    {
        elapsedTimeInS = (esp_timer_get_time() - m_TimeOnPress) / 1000000;
    }

    switch (m_FactoryResetProcessState)
    {
        case FactoryResetProcessState::WAIT:
        {
            if (elapsedTimeInS > m_FactoryButtonConfig.start_time_in_s)
            {
                if(m_pStatusLedIf)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::START;
                    m_pStatusLedIf->Update(status);
                }
                m_FactoryResetProcessState = FactoryResetProcessState::PRESSED_FOR_1SEC;
            }
        }   
        break;

        case FactoryResetProcessState::PRESSED_FOR_1SEC:
        {
            if (elapsedTimeInS > m_FactoryButtonConfig.finish_time_in_s)
            {
                if(m_pStatusLedIf)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::DONE_WAIT_FOR_RELEASE;
                    m_pStatusLedIf->Update(status);
                }
                m_FactoryResetProcessState = FactoryResetProcessState::DONE;
            }
            else if (level)
            {
                if(m_pStatusLedIf)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::CANCEL;
                    m_pStatusLedIf->Update(status);
                }
                m_FactoryResetProcessState = FactoryResetProcessState::WAIT;
            }
        }
        break;

        case FactoryResetProcessState::DONE:
        {
            if (!m_prevLevel && level)
            {
                if(m_pStatusLedIf)
                {
                    StatusLed::Status status;
                    status.type = StatusLed::StatusType::FACTORY_RESET;
                    status.factoryResetState = StatusLed::FactoryResetState::DONE;
                    m_pStatusLedIf->Update(status);
                }

                WifiNatRouterApp::Command cmd;
                cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdFactoryReset;
                m_pWifiNatRouterIf->SendCommand(cmd);
                
                m_FactoryResetProcessState = FactoryResetProcessState::WAIT;           
            }
        }
        break;
    }

    m_prevLevel = level;
}

}

