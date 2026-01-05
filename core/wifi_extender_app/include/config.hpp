#pragma once

#include <string_view>
#include "sdkconfig.h"

static constexpr std::string_view DEFAULT_AP_SSID = "DEF_AP";
static constexpr std::string_view DEFAULT_AP_PASSWORD = "DEF_AP_PASSWORD";

static constexpr std::string_view DEFAULT_STA_SSID = "DEF_STA";
static constexpr std::string_view DEFAULT_STA_PASSWORD = "DEF_STA_PASSWORD";

static constexpr bool ESP32TARGET_S3 = CONFIG_IDF_TARGET_ESP32S3;
static constexpr uint32_t GPIO_LED_ESP32S3 = 38;