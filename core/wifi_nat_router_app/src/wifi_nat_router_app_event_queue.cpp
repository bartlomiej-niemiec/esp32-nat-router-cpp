#include "wifi_nat_router_app_event_queue.hpp"

namespace WifiNatRouterApp
{

WifiNatRouterAppEventQueue::WifiNatRouterAppEventQueue():
    m_MessageQueue(),
    m_QueueStorage(),
    m_MessageQueueBuffer()
{
    m_MessageQueue = xQueueCreateStatic(MESSAGE_QUEUE_SIZE, sizeof(Message), m_MessageQueueBuffer, &m_QueueStorage);
    assert(nullptr != m_MessageQueue);
}

WifiNatRouterAppEventQueue::~WifiNatRouterAppEventQueue()
{
    vQueueDelete(m_MessageQueue);
}

bool WifiNatRouterAppEventQueue::Add(const Message & msg)
{
    return xQueueSend(m_MessageQueue, &msg, 0) == pdTRUE;
}

bool WifiNatRouterAppEventQueue::Receive(Message & msg)
{
    return xQueueReceive(
        m_MessageQueue,
        &msg,
        portMAX_DELAY
    ) == pdTRUE;
}

}