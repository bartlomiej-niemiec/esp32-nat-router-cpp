#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <string_view>
#include <array>

#include "lwip/inet.h"

namespace WifiExtender
{

enum class WifiExtenderState : uint8_t {
    STOPPED = 0,
    STARTED,
    CONNECTING,
    RUNNING,
    STOPPING,
    NEW_CONFIGURATION_PENDING,
    STA_CANNOT_CONNECT,
    STATE_COUNT /// Must be last 
};

struct AccessPointConfig {
    AccessPointConfig();
    AccessPointConfig(std::string ssid, std::string password, uint32_t ipAddress, uint32_t networkmask, int max_clients = 1);
    AccessPointConfig(std::string_view ssid, std::string_view password, uint32_t ipAddress, uint32_t networkmask, int max_clients = 1);
    AccessPointConfig(const AccessPointConfig & config) = default;
    AccessPointConfig(AccessPointConfig & config) = default;
    AccessPointConfig & operator=(AccessPointConfig & apconfig) = default;
    AccessPointConfig & operator=(const AccessPointConfig & apconfig) = default;

    bool operator==(AccessPointConfig const& apconfig) const = default;

    bool operator!=(AccessPointConfig const& apconfig) const = default;

    bool IsValid() const;

    static constexpr uint8_t MAX_SSID_SIZE = 32;
    static constexpr uint8_t MAX_PASSWORD_SIZE = 64;
    std::array<char, MAX_SSID_SIZE> ssid;
    std::array<char, MAX_PASSWORD_SIZE> password;
    int max_clients;
    uint32_t ipAddress;
    uint32_t networkmask;
};

struct StaConfig {
    StaConfig();
    StaConfig(std::string ssid, std::string password);
    StaConfig(std::string_view ssid, std::string_view password);
    StaConfig(StaConfig & config) = default;
    StaConfig(const StaConfig & config) = default;
    StaConfig & operator=(StaConfig & staconfig) = default;
    StaConfig & operator=(const StaConfig & staconfig) = default;

    bool operator==(StaConfig const& staconfig) const = default;

    bool operator!=(StaConfig const& staconfig) const = default;

    bool IsValid() const;

    static constexpr uint8_t MAX_SSID_SIZE = 32;
    static constexpr uint8_t MAX_PASSWORD_SIZE = 64;
    std::array<char, MAX_SSID_SIZE> ssid;
    std::array<char, MAX_PASSWORD_SIZE> password;
};

struct WifiExtenderConfig {
    
    AccessPointConfig apConfig;
    StaConfig         staConfig;

    WifiExtenderConfig();
    WifiExtenderConfig(const AccessPointConfig& ap, const StaConfig& sta);
    WifiExtenderConfig(const WifiExtenderConfig&)            = default;
    WifiExtenderConfig(WifiExtenderConfig&&)                 = default;
    WifiExtenderConfig& operator=(const WifiExtenderConfig&) = default;
    WifiExtenderConfig& operator=(WifiExtenderConfig&&)      = default;

    bool operator==(const WifiExtenderConfig& other) const;

    bool operator!=(const WifiExtenderConfig& other) const;

    static void printConfig(WifiExtenderConfig& config);
};


class WifiExtenderHelpers
{
public:

    static constexpr bool ConvertStringToIpAddress(const char * ipAddrStr, uint32_t & ipAddrNum)
    {
        ip4_addr_t ipAddrT;
        int res = ip4addr_aton(ipAddrStr, &ipAddrT);
        if (res == 1)
        {
            std::memcpy(&ipAddrNum, &ipAddrT, sizeof(ip4_addr_t));
            return true;
        }
        return false;
    }

    static constexpr std::string_view WifiExtenderStaToString(const WifiExtenderState & state)
    {
        switch(state)
        {
            case WifiExtenderState::STARTED:
            {
                return "WifiExtender started";
            }
            break;

            case WifiExtenderState::CONNECTING:
            {
                return "WifiExtender connecting";
            }
            break;

            case WifiExtenderState::RUNNING:
            {
                return "WifiExtender running";
            }
            break;

            case WifiExtenderState::STOPPED:
            {
                return "WifiExtender stopped";
            }
            break;

            case WifiExtenderState::STOPPING:
            {
                return "WifiExtender stopping";
            }
            break; 

            case WifiExtenderState::STA_CANNOT_CONNECT:
            {
                return "WifiExtender STA cannot connect";
            }
            break;

            case WifiExtenderState::NEW_CONFIGURATION_PENDING:
            {
                return "WifiExtender new configuration pending...";
            }
            break;

            case WifiExtenderState::STATE_COUNT:
            {
                return "Last State - should not be use in app code";
            }
            break;
        };
        
        return "WifiExtender unknown state";
    }
};


}