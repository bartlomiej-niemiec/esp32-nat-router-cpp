#pragma once

#include <stdint.h>
#include <string_view>

/// Default users and their credentials, levels
static constexpr std::string_view ADMIN_USER_DEFAULT_NAME ="admin";
static constexpr std::string_view ADMIN_USER_DEFAULT_PASSWORD = "admin";
static constexpr int ADMIN_USER_DEFAULT_LEVEL = 7;

static constexpr std::string_view NON_ADMIN_USER_DEFAULT_NAME ="user";
static constexpr std::string_view NON_ADMIN_USER_DEFAULT_PASSWORD = "user";
static constexpr int NON_ADMIN_USER_DEFAULT_LEVEL = 1;