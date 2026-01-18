#include "wifi_nat_router_app_impl.hpp"

#include "wifi_nat_router_app_config.hpp"



namespace WifiNatRouterApp
{

WifiNatRouterAppImpl::WifiNatRouterAppImpl(WifiNatRouter::WifiNatRouterIf & rWifiIf):
    m_NetworkConfigManager(),
    m_rWifiNatRouter(rWifiIf),
    m_pLed(nullptr),
    m_EventQueue(),
    m_CommandQueue(),
    m_EventDispatcher(m_EventQueue),
    m_MainTask(nullptr)
{
    if (ENABLE_RGB_LED)
    {
        m_pLed = new (std::nothrow) NetworkStatusLed::NetworkStatusLed(RGB_LED_GPIO_PIN);
    }

    m_rWifiNatRouter.RegisterListener(&m_EventDispatcher);

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

WifiNatRouterAppImpl::~WifiNatRouterAppImpl()
{
    delete m_pLed;
    vTaskDelete(m_MainTask);
}

void WifiNatRouterAppImpl::MainLoop(void *pArg)
{
    WifiNatRouterAppImpl * pInstance = reinterpret_cast<WifiNatRouterAppImpl*>(pArg);
    constexpr uint32_t TASK_DELAY_MS = 10;
    constexpr uint32_t TASK_DELAY_TICKS = pdMS_TO_TICKS(TASK_DELAY_MS);
    pInstance->m_rWifiNatRouter.Startup(
        {pInstance->m_NetworkConfigManager.GetApConfig(), pInstance->m_NetworkConfigManager.GetStaConfig()}
    );

    for (;;)
    {
        pInstance->ProcessEventQueue();
        pInstance->ProcessCommandQueue();
        vTaskDelay(TASK_DELAY_TICKS);
    }
}

}