#include "wifi_event_monitor.hpp"
#include "wifi_extender_if/wifi_extender_factory.hpp"

#include "esp_log.h"

#include <new>

WifiEventMonitor::WifiEventMonitor():
    m_WifiEventDispatcher(nullptr),
    m_WifiExtenderEventQueue(nullptr),
    m_WifiMonitoraskHandle(nullptr)
{
    m_WifiExtenderEventQueue = xQueueCreate(WIFI_EXTENDER_QUEUE_SIZE, sizeof(WifiExtender::WifiExtenderState));
    assert(nullptr != m_WifiExtenderEventQueue);
    m_WifiEventDispatcher = new (std::nothrow) WifiEventDispatcher(m_WifiExtenderEventQueue);
    assert(nullptr != m_WifiEventDispatcher);
}

WifiEventMonitor::~WifiEventMonitor()
{
    delete m_WifiEventDispatcher;
    vQueueDelete(m_WifiExtenderEventQueue);
}

bool WifiEventMonitor::Subscribe(const ListenerFunction & function)
{
    m_Listeners.push_back(function);
    return true;
}

bool WifiEventMonitor::Startup()
{
    if (m_WifiMonitoraskHandle != nullptr) return false;

    WifiExtender::WifiExtenderIf & pWifiExtender = WifiExtender::WifiExtenderFactory::GetInstance().GetWifiExtender();
    xTaskCreate(
        WifiMonitorObserverTask,
        m_pTaskName.data(),
        TASK_STACK_SIZE,
        this,
        TASK_PRIO,
        &m_WifiMonitoraskHandle
    );
    assert(nullptr != m_WifiMonitoraskHandle);
    pWifiExtender.RegisterListener(m_WifiEventDispatcher);

    return true;
}

void WifiEventMonitor::WifiMonitorObserverTask(void *pArg)
{
    assert(nullptr != pArg);
    WifiEventMonitor * pInstance = reinterpret_cast<WifiEventMonitor *>(pArg);
    WifiExtender::WifiExtenderState state;
    while (1)
    {
        if (xQueueReceive(
            pInstance->m_WifiExtenderEventQueue,
            &state,
            portMAX_DELAY
            ) == pdTRUE )
        {
            for (auto & subcriber : pInstance->m_Listeners)
            {
                subcriber(state);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}