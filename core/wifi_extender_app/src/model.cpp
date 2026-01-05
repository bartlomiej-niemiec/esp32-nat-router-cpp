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

int my_glue_authenticate(const char *user, const char *pass) {
    ESP_LOGI("MONGOOSE", "User: %s, Password: %s", user, pass);
    int level = 0; // Authentication failure
    if (strcmp(user, "admin") == 0 && strcmp(pass, "admin") == 0) {
        level = 7;  // Administrator
    } else if (strcmp(user, "user") == 0 && strcmp(pass, "user") == 0) {
        level = 3;  // Ordinary dude
    }
    return level;
}

static void run_mongoose(void *pArg) {
    ESP_LOGI("MONGOOSE", "run_mongoose started");
    mongoose_init();
    mongoose_set_auth_handler(my_glue_authenticate);
    mg_log_set(MG_LL_DEBUG);  // Set log level to debug
    for (;;) {                // Infinite event loop
        mongoose_poll();   // Process network events
    }
}

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
   
    if (ESP32TARGET_S3)
    {
        m_Led = new (std::nothrow) NetworkStatusLed::NetworkStatusLed(GPIO_LED_ESP32S3);
    }

    m_WifiExtenderEventQueue = xQueueCreate(WIFI_EXTENDER_QUEUE_SIZE, sizeof(WifiExtender::WifiExtenderState));
    assert(nullptr != m_WifiExtenderEventQueue);
    static DispatchEventListener logEventListener(m_WifiExtenderEventQueue);
    pWifiExtender->RegisterListener(&logEventListener);

    pWifiExtender->Startup(config);
    WifiExtender::WifiExtenderState state;

    while(true)
    {
        if (xQueueReceive(
            m_WifiExtenderEventQueue,
            &state,
            portMAX_DELAY
            ) == pdTRUE )
        {
            m_Led->Update(state);
            if ((state == WifiExtender::WifiExtenderState::CONNECTING ||
                state == WifiExtender::WifiExtenderState::RUNNING) && !isMongooseRunning)
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
        vTaskDelay(pdMS_TO_TICKS(10));
    };
}