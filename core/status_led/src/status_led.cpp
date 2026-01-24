#include "status_led/status_led.hpp"
#include "rgbled_if/rgbled_factory.hpp"
#include <assert.h>
#include <type_traits>

namespace StatusLed
{

StatusLed::StatusLed(const uint32_t gpio_pin_num):
    m_pStatusLed(nullptr),
    m_CachedRouterState(),
    m_CachedInternetAccess(false),
    m_State(StatusLedState::NETWORK_STATUS),
    m_MainTask(nullptr),
    m_MessageQueue(nullptr),
    m_QueueStorage(),
    m_MessageQueueBuffer()
{
    auto rgbledfactory = RgbLed::RgbLedFactory::GetInstance();
    m_pStatusLed = rgbledfactory.Create(gpio_pin_num);
    assert(nullptr != m_pStatusLed);

    m_MessageQueue = xQueueCreateStatic(MESSAGE_QUEUE_SIZE, sizeof(Status), m_MessageQueueBuffer, &m_QueueStorage);
    assert(nullptr != m_MessageQueue);

    xTaskCreate(
            MainLoop,
            TASK_NAME.data(),
            TASK_STACK_SIZE,
            this,
            TASK_PRIORITY,
            &m_MainTask
    );
    assert(nullptr != m_MainTask);
}
 
StatusLed::~StatusLed()
{
    vQueueDelete(m_MessageQueue);
}

bool StatusLed::Update (const Status & status)
{
    return xQueueSend(m_MessageQueue, &status, pdMS_TO_TICKS(5)) == pdTRUE;
}

void StatusLed::MainLoop(void *pArg)
{
    assert(pArg != nullptr);
    StatusLed * pInstance = reinterpret_cast<StatusLed*>(pArg);
    Status msg;
    for (;;)
    {
        if (xQueueReceive(
            pInstance->m_MessageQueue,
            &msg,
            portMAX_DELAY
        ) == pdTRUE)
        {
            switch(pInstance->m_State)
            {
                case StatusLedState::NETWORK_STATUS:
                {
                    switch(msg.type)
                    {
                        case StatusType::NETWORK_STATUS_UPDATE:
                        {
                            pInstance->m_CachedRouterState = msg.routerState;
                            pInstance->UpdateLedRouterState(msg.routerState);
                        }
                        break;

                        case StatusType::INTERNET_ACCESS:
                        {
                            pInstance->m_CachedInternetAccess = msg.internetAvailable;
                            pInstance->UpdateLedInternetAccess(msg.internetAvailable);
                        }
                        break;

                        case StatusType::FACTORY_RESET:
                        {
                            if (msg.factoryResetState == FactoryResetState::START)
                            {
                                pInstance->m_State = StatusLedState::FACTORY_RESET_PENDING;
                                pInstance->UpdateLedFactoryReset(FactoryResetState::START);
                            }
                        }
                        break;

                    };
                }
                break;

                case StatusLedState::FACTORY_RESET_PENDING:
                {
                    switch(msg.type)
                    {
                        case StatusType::NETWORK_STATUS_UPDATE:
                        {
                            pInstance->m_CachedRouterState = msg.routerState;
                        }
                        break;

                        case StatusType::INTERNET_ACCESS:
                        {
                           pInstance->m_CachedInternetAccess = msg.internetAvailable;
                        }
                        break;

                        case StatusType::FACTORY_RESET:
                        {
                            if (
                            (msg.factoryResetState == FactoryResetState::CANCEL) ||
                            (msg.factoryResetState == FactoryResetState::DONE)
                            )
                            {
                                if (!(pInstance->m_CachedInternetAccess))
                                {
                                    pInstance->UpdateLedRouterState(pInstance->m_CachedRouterState);
                                }
                                else
                                {
                                    pInstance->UpdateLedInternetAccess(true);
                                }
                                pInstance->m_State = StatusLedState::NETWORK_STATUS;
                            }
                        }
                        break;

                    };
                }   
                break;
            };
        }
    }
}

void StatusLed::UpdateLedRouterState(WifiNatRouter::WifiNatRouterState state)
{
    std::visit(
            Visitor2{
                [&](const SolidConfig& cfg) {
                    m_pStatusLed->Solid(cfg.color);
                },
                [&](const BlinkConfig& cfg) {
                    m_pStatusLed->Blink(cfg.color, cfg.blink_freq);
                }
            },
            m_RouterStateLedMatrix[ToIndex(state)]
        );
}

void StatusLed::UpdateLedInternetAccess(bool internetAvailable)
{
    if (internetAvailable)
    {
        constexpr RgbLed::RgbColor internetAccessColor = RgbLed::RgbColorCreator::Create(RgbLed::Color::Green);
        m_pStatusLed->Solid(internetAccessColor);
    }
    else
    {
        UpdateLedRouterState(m_CachedRouterState);
    }
}

void StatusLed::UpdateLedFactoryReset(FactoryResetState factoryResetState)
{
    //TODO
}




}
  