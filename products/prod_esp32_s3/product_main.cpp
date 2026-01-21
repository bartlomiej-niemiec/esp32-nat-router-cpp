#include "data_storer_if/data_storer.hpp"
#include "wifi_nat_router_app.hpp"
#include "wifi_nat_router_if/wifi_nat_router_factory.hpp"
#include "webserver/webserver.hpp"
#include "freertos/Freertos.h"

void product_main(void)
{
    DataStorage::DataStorer::Init();

    WifiNatRouterApp::WifiNatRouterApp app(WifiNatRouter::WifiNatRouterFactory::GetInstance().GetWifiNatRouter());
    WebServer & webServer = WebServer::GetInstance();
    webServer.Startup(&app.GetAppIf());

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}