#pragma once

#include "wifi_extender_if/wifi_extender_if.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "memory"

class WifiEventMonitor
{
    public:

        WifiEventMonitor();

        ~WifiEventMonitor();

        bool Startup();

        using ListenerFunction = std::function<void(WifiExtender::WifiExtenderState event)>;

        bool Subscribe(const ListenerFunction & function);

    private:

        class WifiEventDispatcher:
                public WifiExtender::EventListener
        {
                public:

                    WifiEventDispatcher(QueueHandle_t q):
                        m_QueueHandle(q)
                    {};

                    void Callback(WifiExtender::WifiExtenderState event) override
                    {
                        xQueueGenericSend(m_QueueHandle, (void * ) &event, ( TickType_t ) 0, queueSEND_TO_BACK );
                    };
                
                private:
                    QueueHandle_t m_QueueHandle; 
        };
    
    static constexpr uint32_t WIFI_EXTENDER_QUEUE_SIZE = 16;
    WifiEventDispatcher * m_WifiEventDispatcher;
    QueueHandle_t m_WifiExtenderEventQueue;
    std::vector<ListenerFunction> m_Listeners;
    
    static constexpr int TASK_PRIO = 3;
    static constexpr int TASK_STACK_SIZE = 2048;
    static constexpr std::string_view m_pTaskName = "WifiMonitorTask";
    TaskHandle_t m_WifiMonitoraskHandle;


    static void WifiMonitorObserverTask(void *pArg);
};