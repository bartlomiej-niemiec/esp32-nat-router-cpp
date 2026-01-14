cmake_minimum_required(VERSION 3.16)

idf_component_register(
                        SRCS  "./src/wifi_nat_router_app.cpp"
                              "./src/wifi_event_monitor.cpp"
                              "./src/network_status_led.cpp"
                              "./src/webserver_srvs.cpp"
                              "./src/network_config_manager.cpp"
                              "./src/webserver.cpp"
                        INCLUDE_DIRS "./include"
                        REQUIRES    wifi_nat_router_if
                                    wifi_nat_router_espidf_impl
                                    rgbled_if
                                    rgbled_sk68xxminihs
                                    data_storer_if
                                    data_storer_nvs_impl
                                    mongoose         
                                    user_credential_manager
                    )