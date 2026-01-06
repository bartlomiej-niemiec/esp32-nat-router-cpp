#include "controller.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"

#include "esp_log.h"
#include "config.hpp"

void Controller::Startup()
{
    using namespace WifiExtender;

    WifiExtenderConfig config(m_NetworkConfigManager.GetApConfig(), m_NetworkConfigManager.GetStaConfig());
    m_pWifiExtender = &WifiExtenderFactory::GetInstance().GetWifiExtender();
   
    if (ESP32S3_TARGET)
    {
        m_Led = new (std::nothrow) NetworkStatusLed::NetworkStatusLed(GPIO_BUILT_RGB_LED_ESP32S3);
    }

    m_WifiExtenderEventQueue = xQueueCreate(WIFI_EXTENDER_QUEUE_SIZE, sizeof(WifiExtender::WifiExtenderState));
    assert(nullptr != m_WifiExtenderEventQueue);
    WifiEventDispatcher logEventListener(m_WifiExtenderEventQueue);

    m_pWifiExtender->RegisterListener(&logEventListener);
    m_pWifiExtender->Startup(config);
    constexpr uint32_t DELAY_MS = 200;
    vTaskDelay(pdMS_TO_TICKS(DELAY_MS));

    WebServer & webServerInstance = WebServer::GetInstance();
    webServerInstance.Init(
        &UserCredential::UserCredentialManager::GetInstance(),
        m_pWifiExtender
    );
    webServerInstance.Startup();

    WifiExtender::WifiExtenderState state;
    while(true)
    {
        if (xQueueReceive(
            m_WifiExtenderEventQueue,
            &state,
            portMAX_DELAY
            ) == pdTRUE )
        {
            if (m_Led)
            {
                m_Led->Update(state);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    };
}