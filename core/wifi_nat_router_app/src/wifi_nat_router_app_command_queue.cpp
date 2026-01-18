#include "wifi_nat_router_app_command_queue.hpp"

namespace WifiNatRouterApp
{

WifiNatRouterAppCommandQueue::WifiNatRouterAppCommandQueue():
    m_MessageQueue(),
    m_QueueStorage(),
    m_MessageQueueBuffer()
{
    m_MessageQueue = xQueueCreateStatic(MESSAGE_QUEUE_SIZE, sizeof(Command), m_MessageQueueBuffer, &m_QueueStorage);
    assert(nullptr != m_MessageQueue);
}

WifiNatRouterAppCommandQueue::~WifiNatRouterAppCommandQueue()
{
    vQueueDelete(m_MessageQueue);
}

bool WifiNatRouterAppCommandQueue::Add(const Command & msg)
{
    return xQueueSend(m_MessageQueue, &msg, 0) == pdTRUE;
}

bool WifiNatRouterAppCommandQueue::Receive(Command & msg)
{
    return xQueueReceive(
        m_MessageQueue,
        &msg,
        0
    ) == pdTRUE;
}

}