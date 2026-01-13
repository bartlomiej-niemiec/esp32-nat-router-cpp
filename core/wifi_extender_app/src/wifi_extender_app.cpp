#include "wifi_extender_app.hpp"

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "wifi_extender_if/wifi_extender_factory.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_scanner_types.hpp"

#include "user_credential_manager/user_credential_manager.hpp"

#include "network_config_manager.hpp"
#include "network_status_led.hpp"

#include "webserver.hpp"
#include "webserver_srvs.hpp"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"

#include "esp_log.h"
#include "config.hpp"

#include "wifi_event_monitor.hpp"

#include "data_storer_if/data_storer.hpp"

namespace WifiExtenderApp
{

void Init()
{
    DataStorage::DataStorer::Init();
}

void Startup()
{
    NetworkConfigManager m_NetworkConfigManager;

    WifiExtender::WifiExtenderIf & rWifiExtender = WifiExtender::WifiExtenderFactory::GetInstance().GetWifiExtender();

    NetworkStatusLed::NetworkStatusLed * m_Led = nullptr;
    WifiEventMonitor wifiEventMonitor;
    if (ESP32S3_TARGET)
    {
        m_Led = new (std::nothrow) NetworkStatusLed::NetworkStatusLed(GPIO_BUILT_RGB_LED_ESP32S3);
        wifiEventMonitor.Subscribe([&m_Led](WifiExtender::WifiExtenderState state){m_Led->Update(state);});
        wifiEventMonitor.Startup();
    }
    
    rWifiExtender.Startup({m_NetworkConfigManager.GetApConfig(), m_NetworkConfigManager.GetStaConfig()});

    constexpr uint32_t DELAY_MS = 200;
    vTaskDelay(pdMS_TO_TICKS(DELAY_MS));

    WebServerServices::Init(
        &(UserCredential::UserCredentialManager::GetInstance()),
        &rWifiExtender,
        &m_NetworkConfigManager,
        &wifiEventMonitor
    );
    
    WebServer & webServerInstance = WebServer::GetInstance();
    webServerInstance.Startup();

    for (;;)
    {
        vTaskDelay(pdTICKS_TO_MS(2000));
    }
}


}