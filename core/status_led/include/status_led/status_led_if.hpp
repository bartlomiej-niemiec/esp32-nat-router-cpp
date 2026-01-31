#pragma once

#include "wifi_nat_router_if/wifi_nat_router_if.hpp"

namespace StatusLed
{

enum class StatusType {
    NETWORK_STATUS_UPDATE,
    INTERNET_ACCESS,
    FACTORY_RESET
};

enum class FactoryResetState : uint8_t
{
    START = 0,
    CANCEL,
    DONE_WAIT_FOR_RELEASE,
    DONE,
    MAX_STATE
};

struct Status {
    StatusType type;
    WifiNatRouter::WifiNatRouterState routerState;
    bool internetAvailable;
    FactoryResetState factoryResetState;
};

class StatusLedIf
{
    public:
        virtual bool Update (const Status & status) = 0;

        virtual ~StatusLedIf() = default;
};

}