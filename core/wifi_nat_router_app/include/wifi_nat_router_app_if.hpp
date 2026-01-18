#pragma once 

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "wifi_nat_router_if/wifi_scanner_scanner_types.hpp"

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
    WifiNatRouter::WifiNetwork scannedNetworks[WIFI_NETWORK_SCAN_LIMIT];
    uint8_t scannedCount;
    WifiNatRouter::ScannerState scanState;
    bool configApplyInProgress;
    bool internetAccess;
    int noApClients;
};


class WifiNatRouterAppIf{

    public:
        virtual ~WifiNatRouterAppIf() = default;
        virtual bool SendCommand(const Command & cmd) const = 0;
        virtual bool TryGetSnapshot(AppSnapshot& out) const = 0;

};

}