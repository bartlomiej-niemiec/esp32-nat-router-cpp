cmake_minimum_required(VERSION 3.16)

idf_component_register(SRCS "product_main.cpp"
                       INCLUDE_DIRS "./"
                       REQUIRES wifi_nat_router_app
                                wifi_nat_router_espidf_impl
                                rgbled_sk68xxminihs_impl
                                data_storer_nvs_impl
                                webserver
                                status_led
                                factory_reset_pb
)