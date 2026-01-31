#pragma once

#include <stdint.h>
#include <string_view>

/// RGB Status LED
static constexpr bool ENABLE_RGB_LED = true;
static constexpr uint32_t GPIO_BUILT_RGB_LED_ESP32S3 = 38;
static constexpr uint32_t RGB_LED_GPIO_PIN = GPIO_BUILT_RGB_LED_ESP32S3;

/// Factory Reset Pb
static constexpr bool ENABLE_FACTORY_RESET_PB = true;
static constexpr uint32_t FACTORY_RESET_GPIO_PIN = 10;
static constexpr uint8_t FACTORY_RESET_PB_PRESS_TIME_TO_BLINK_LED = 1;
static constexpr uint8_t FACTORY_RESET_PB_PRESS_TIME_TO_REQUEST = 8;