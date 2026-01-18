#pragma once 

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "wifi_nat_router_if/wifi_scanner_scanner_types.hpp"
#include <array>

namespace WifiNatRouterApp
{

enum class WifiNatRouterCmd : uint8_t {
    CmdStartScan,
    CmdSetStaConfig,
    CmdSetApConfig,
    CmdApplyNetConfig,
    CmdFactoryReset
};

struct Command {
    Command(){};

    Command(WifiNatRouterCmd c, WifiNatRouter::AccessPointConfig ap = {}, WifiNatRouter::StaConfig sta = {}):
        cmd(c),
        apConfig(ap),
        staConfig(sta)
    {};

    WifiNatRouterCmd cmd;
    WifiNatRouter::AccessPointConfig apConfig;
    WifiNatRouter::StaConfig staConfig;
};

struct AppSnapshot{
    static constexpr int WIFI_NETWORK_SCAN_LIMIT = 8;

    WifiNatRouter::WifiNatRouterConfig config;
    WifiNatRouter::WifiNatRouterState routerState;
    std::array<WifiNatRouter::WifiNetwork, WIFI_NETWORK_SCAN_LIMIT> scannedNetworks;
    uint8_t scannedCount;
    WifiNatRouter::ScannerState scanState;
    bool configApplyInProgress;
    bool internetAccess;
    int noApClients;

    bool operator==(const AppSnapshot& o) const {
        return config == o.config &&
                routerState == o.routerState &&
                scannedNetworks == o.scannedNetworks &&
                scannedCount == o.scannedCount &&
                scanState == o.scanState &&
                configApplyInProgress == o.configApplyInProgress &&
                internetAccess == o.internetAccess &&
                noApClients == o.noApClients;
    }
};


class WifiNatRouterAppIf{

    public:
        virtual ~WifiNatRouterAppIf() = default;
        virtual bool SendCommand(const Command & cmd) = 0;
        virtual bool TryGetSnapshot(AppSnapshot& out) const = 0;

};

}