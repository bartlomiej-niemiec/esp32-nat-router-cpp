#pragma once

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "esp_netif.h"
#include "lwip/stats.h"

namespace WifiNatRouter
{

class WifiSta{

    public:

        WifiSta();

        ~WifiSta();

        enum class State {
            NOT_INITIALIZED,
            INITIALIZED,
            CONFIGURED,
            CONNECTING,
            CONNECTED,
            CONNECTED_AND_GOT_IP,
            DISCONNECTED,
            STOPPED,
            ERROR
        };

        bool Init();

        bool SetConfig(const StaConfig &sta_config);

        esp_netif_dns_info_t GetDnsInfo();

        esp_netif_ip_info_t GetIpInfo();

        void SetState(WifiSta::State state);

        void SetDefaultNetIf();

        WifiSta::State GetState() const;

    private:

        esp_netif_t *m_sta_netif;

        State m_State;

};

}