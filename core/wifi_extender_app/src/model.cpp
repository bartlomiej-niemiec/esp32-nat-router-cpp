#include "model.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"

#include "esp_log.h"
#include "config.hpp"

static volatile bool isMongooseRunning = false;
static constexpr char m_pTaskName[] = "MongooseWebServer";
static constexpr int MONGOOSE_TASK_STACK_SIZE = 4096 * 2;
static constexpr int MONGOOSE_TASK_PRIO = 3;
static TaskHandle_t m_MongooseTaskHandle = nullptr;

static void run_mongoose(void *pArg) {
    ESP_LOGI("MONGOOSE", "run_mongoose started");
    mongoose_init();
    mg_log_set(MG_LL_DEBUG);  // Set log level to debug
    for (;;) {                // Infinite event loop
        mongoose_poll();   // Process network events
    }
}

class DispatchEventListener:
    public WifiExtender::EventListener
{
    public:

        void Callback(WifiExtender::WifiExtenderState event) override
        {
            ESP_LOGI("WifiExtender", "State: %s", WifiExtender::WifiExtenderHelpers::WifiExtenderStaToString(event).data());
            if ((event == WifiExtender::WifiExtenderState::CONNECTING ||
                event == WifiExtender::WifiExtenderState::RUNNING) && !isMongooseRunning)
            {
                xTaskCreate(
                    run_mongoose,
                    m_pTaskName,
                    MONGOOSE_TASK_STACK_SIZE,
                    nullptr,
                    MONGOOSE_TASK_PRIO,
                    &m_MongooseTaskHandle
                );
                assert(nullptr != m_MongooseTaskHandle);
                isMongooseRunning = true;
            }
        }
};

void Model::Startup()
{
    using namespace WifiExtender;

    const AccessPointConfig apConfig(
        DEFAULT_AP_SSID,
        DEFAULT_AP_PASSWORD
    );

    const StaConfig staConfig(
        DEFAULT_STA_SSID,
        DEFAULT_STA_PASSWORD
    );

    auto printApConfig = [](AccessPointConfig ap){
        ESP_LOGI("NvsApConfig", "ssid: %s", ap.ssid.data());
        ESP_LOGI("NvsApConfig", "password: %s", ap.password.data());
    };

    WifiExtenderConfig config(apConfig, staConfig);
    pWifiExtender = &WifiExtenderFactory::GetInstance().GetWifiExtender();
    pWifiScannerIf = pWifiExtender->GetScanner();
    static DispatchEventListener logEventListener;
    pWifiExtender->RegisterListener(&logEventListener);
    if (ESP32TARGET_S3)
    {
        m_Led = new (std::nothrow) NetworkStatusLed::NetworkStatusLed(GPIO_LED_ESP32S3);
        m_WifiListenerLed = new (std::nothrow) NetworkStatusLed::NetworkLedEventListener(m_Led);
        pWifiExtender->RegisterListener(m_WifiListenerLed);
    }
    pWifiExtender->Startup(config);
    while(true)
    { 
        vTaskDelay(pdMS_TO_TICKS(1500));
    };
}