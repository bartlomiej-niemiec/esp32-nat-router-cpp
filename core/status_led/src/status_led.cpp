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
    m_MessageQueue(nullptr),
    m_QueueStorage(),
    m_MessageQueueBuffer()
{
    auto rgbledfactory = RgbLed::RgbLedFactory::GetInstance();
    m_pStatusLed = rgbledfactory.Create(gpio_pin_num);
    assert(nullptr != m_pStatusLed);

    m_MessageQueue = xQueueCreateStatic(MESSAGE_QUEUE_SIZE, sizeof(Status), m_MessageQueueBuffer, &m_QueueStorage);
    assert(nullptr != m_MessageQueue);
}
 
StatusLed::~StatusLed()
{
    vQueueDelete(m_MessageQueue);
}

bool StatusLed::Update (const Status & status)
{
    return xQueueSend(m_MessageQueue, &status, 0) == pdTRUE;
}

void StatusLed::MainLoop()
{
    Status msg;
    static constexpr uint32_t MAX_EVENT_PROCESSING_PER_LOOP = 2;

    for (int i = 0; i < MAX_EVENT_PROCESSING_PER_LOOP;i++)
    {
        if (xQueueReceive(
                m_MessageQueue,
                &msg,
                0
        ) == pdTRUE)
        {
            switch(m_State)
                {
                    case StatusLedState::NETWORK_STATUS:
                    {
                        switch(msg.type)
                        {
                            case StatusType::NETWORK_STATUS_UPDATE:
                            {
                                m_CachedRouterState = msg.routerState;
                                UpdateLedRouterState(msg.routerState);
                            }
                            break;

                            case StatusType::INTERNET_ACCESS:
                            {
                                m_CachedInternetAccess = msg.internetAvailable;
                                UpdateLedInternetAccess(msg.internetAvailable);
                            }
                            break;

                            case StatusType::FACTORY_RESET:
                            {
                                if (msg.factoryResetState == FactoryResetState::START)
                                {
                                    UpdateLedFactoryReset(FactoryResetState::START);
                                    m_State = StatusLedState::FACTORY_RESET_PENDING;
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
                                m_CachedRouterState = msg.routerState;
                            }
                            break;

                            case StatusType::INTERNET_ACCESS:
                            {
                            m_CachedInternetAccess = msg.internetAvailable;
                            }
                            break;

                            case StatusType::FACTORY_RESET:
                            {
                                if (
                                (msg.factoryResetState == FactoryResetState::CANCEL)
                                )
                                {
                                    if (!(m_CachedInternetAccess))
                                    {
                                        UpdateLedRouterState(m_CachedRouterState);
                                    }
                                    else
                                    {
                                        UpdateLedInternetAccess(true);
                                    }
                                    m_State = StatusLedState::NETWORK_STATUS;
                                }
                                else if (msg.factoryResetState == FactoryResetState::DONE_WAIT_FOR_RELEASE)
                                {
                                    UpdateLedFactoryReset(FactoryResetState::DONE_WAIT_FOR_RELEASE);
                                }
                                else if (msg.factoryResetState == FactoryResetState::DONE)
                                {
                                    UpdateLedRouterState(m_CachedRouterState);
                                    m_State = StatusLedState::NETWORK_STATUS;
                                }
                            }
                            break;

                        };
                    }   
                    break;
            };
        }
        else
        {
            // No more events
            break;
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
    const auto & opt = m_FactoryResetLedMatrix[ToIndex(factoryResetState)];
    if (!opt.has_value()) {
        return;
    }

    std::visit(
            Visitor2{
                [&](const SolidConfig& cfg) {
                    m_pStatusLed->Solid(cfg.color);
                },
                [&](const BlinkConfig& cfg) {
                    m_pStatusLed->Blink(cfg.color, cfg.blink_freq);
                }
            },
            opt.value()
    );
}




}
  