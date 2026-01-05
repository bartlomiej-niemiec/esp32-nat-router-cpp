#include "wifi_extender_app.hpp"

extern "C" void app_main(void)
{
    WifiExtenderApp::Init();
    WifiExtenderApp::Startup();
}