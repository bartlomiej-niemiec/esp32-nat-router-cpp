cmake_minimum_required(VERSION 3.16)

set(DataStorerImpl Nvs)

set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_SOURCE_DIR}/main
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/third-party/mongoose
    ${CMAKE_SOURCE_DIR}/components/wifi_nat_router/wifi_nat_router_if
    ${CMAKE_SOURCE_DIR}/components/wifi_nat_router/wifi_nat_router_espidf_impl
    ${CMAKE_SOURCE_DIR}/components/rgbled/rgbled_if
    ${CMAKE_SOURCE_DIR}/components/data_storer/data_storer_if
    ${CMAKE_SOURCE_DIR}/components/user_credential_manager
    ${CMAKE_SOURCE_DIR}/core/internetactivitymonitor
    ${CMAKE_SOURCE_DIR}/core/wifi_nat_router_app
)

if("${DataStorerImpl}" STREQUAL "Nvs")
    list(APPEND EXTRA_COMPONENT_DIRS
        ${CMAKE_SOURCE_DIR}/components/data_storer/data_storer_nvs_impl
    )
endif()

list(APPEND EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/components/rgbled/rgbled_sk68xxminihs
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wifi_nat_router)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
