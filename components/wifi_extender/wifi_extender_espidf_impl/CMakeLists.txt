cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS "./src/wifi_ap.cpp"
        "./src/wifi_sta.cpp"
        "./src/wifi_extender_factory_impl.cpp"
        "./src/wifi_extender_impl.cpp"
        "./src/wifi_manager.cpp"
        "./src/wifi_manager_context.cpp"
        "./src/wifi_scanner.cpp"
    PRIV_REQUIRES esp_wifi "wifi_extender_if"
                            "utils"
                            esp_netif
                            esp_timer
                            nvs_flash
)