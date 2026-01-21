cmake_minimum_required(VERSION 3.16)

idf_component_register(
        SRCS "./src/sk68xxminihs_factory.cpp"
             "./src/rgbled_sk68_impl.cpp"
        PRIV_REQUIRES nvs_flash esp_driver_rmt esp_timer
                      "rgbled_if" "utils"
)