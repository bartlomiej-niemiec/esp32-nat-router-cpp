cmake_minimum_required(VERSION 3.16)

idf_component_register(
                        SRCS  "./src/internet_activity_monitor.cpp"
                        INCLUDE_DIRS "./include"
                        REQUIRES    lwip
                                    wifi_nat_router_app
                    )