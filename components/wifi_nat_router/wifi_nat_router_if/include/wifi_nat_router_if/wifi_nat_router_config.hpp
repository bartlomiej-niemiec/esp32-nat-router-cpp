#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <string_view>
#include <array>

#include "lwip/inet.h"

namespace WifiNatRouter
{

enum class WifiNatRouterState : uint8_t {
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

struct WifiNatRouterConfig {
    
    AccessPointConfig apConfig;
    StaConfig         staConfig;

    WifiNatRouterConfig();
    WifiNatRouterConfig(const AccessPointConfig& ap, const StaConfig& sta);
    WifiNatRouterConfig(const WifiNatRouterConfig&)            = default;
    WifiNatRouterConfig(WifiNatRouterConfig&&)                 = default;
    WifiNatRouterConfig& operator=(const WifiNatRouterConfig&) = default;
    WifiNatRouterConfig& operator=(WifiNatRouterConfig&&)      = default;

    bool operator==(const WifiNatRouterConfig& other) const;

    bool operator!=(const WifiNatRouterConfig& other) const;

    static void printConfig(WifiNatRouterConfig& config);
};


class WifiNatRouterHelpers
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

    static constexpr uint8_t MAX_IP_ADDRESS_STRING_SIZE = 16;

    static constexpr bool ConvertU32ToIpAddressString(char ipAddrStr[MAX_IP_ADDRESS_STRING_SIZE], uint32_t ipAddrNum)
    {

        ip4_addr_t ipAddrT = {.addr = ipAddrNum};
        strncpy(ipAddrStr, ip4addr_ntoa(&ipAddrT), 15);
        ipAddrStr[15] = '\0';
        return true;
    }

    static constexpr std::string_view WifiNatRouterStaToString(const WifiNatRouterState & state)
    {
        switch(state)
        {
            case WifiNatRouterState::STARTED:
            {
                return "WifiNatRouter started";
            }
            break;

            case WifiNatRouterState::CONNECTING:
            {
                return "WifiNatRouter connecting";
            }
            break;

            case WifiNatRouterState::RUNNING:
            {
                return "WifiNatRouter running";
            }
            break;

            case WifiNatRouterState::STOPPED:
            {
                return "WifiNatRouter stopped";
            }
            break;

            case WifiNatRouterState::STOPPING:
            {
                return "WifiNatRouter stopping";
            }
            break; 

            case WifiNatRouterState::STA_CANNOT_CONNECT:
            {
                return "WifiNatRouter STA cannot connect";
            }
            break;

            case WifiNatRouterState::NEW_CONFIGURATION_PENDING:
            {
                return "WifiNatRouter new configuration pending...";
            }
            break;

            case WifiNatRouterState::STATE_COUNT:
            {
                return "Last State - should not be use in app code";
            }
            break;
        };
        
        return "WifiNatRouter unknown state";
    }
};


}