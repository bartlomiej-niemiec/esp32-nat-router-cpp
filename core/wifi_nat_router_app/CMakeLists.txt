cmake_minimum_required(VERSION 3.16)

idf_component_register(
                        SRCS  "./src/wifi_nat_router_app.cpp"
                              "./src/network_status_led.cpp"
                              "./src/network_config_manager.cpp"
                              "./src/wifi_nat_router_app_command_queue.cpp"
                              "./src/wifi_nat_router_app_event_queue.cpp"
                              "./src/wifi_nat_router_app_impl.cpp"
                              "./src/internet_activity_monitor.cpp"
                        INCLUDE_DIRS "./include"
                        REQUIRES    wifi_nat_router_if
                                    wifi_nat_router_espidf_impl
                                    rgbled_if
                                    rgbled_sk68xxminihs
                                    data_storer_if
                                    data_storer_nvs_impl
                                    esp_timer         
)