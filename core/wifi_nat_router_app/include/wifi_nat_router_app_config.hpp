#pragma once

#include <stdint.h>
#include <string_view>


/// Wifi
static constexpr uint8_t ESP_IDF_MINIMAL_PASSWORD_SIZE = 8;

static constexpr std::string_view DEFAULT_AP_SSID = "DEF_AP";
static constexpr std::string_view DEFAULT_AP_PASSWORD = "DEF_AP_PASSWORD";
static constexpr std::string_view DEFAULT_AP_IP_ADDR = "192.168.50.1";
static constexpr std::string_view DEFAULT_AP_NETMASK = "255.255.255.0";

static constexpr std::string_view DEFAULT_STA_SSID = "DEF_STA";
static constexpr std::string_view DEFAULT_STA_PASSWORD = "DEF_STA_PASSWORD";

/// NAT Router RGB Status LED
static constexpr bool ENABLE_RGB_LED = true;
static constexpr uint32_t GPIO_BUILT_RGB_LED_ESP32S3 = 38;
static constexpr uint32_t RGB_LED_GPIO_PIN = GPIO_BUILT_RGB_LED_ESP32S3;