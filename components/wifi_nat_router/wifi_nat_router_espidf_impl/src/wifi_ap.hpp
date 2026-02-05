#pragma once

#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "esp_netif_types.h"
#include "lwip/stats.h"

namespace WifiNatRouter
{

class WifiAp{

    public:
    
        WifiAp();

        ~WifiAp();

        bool Init();

        bool SetConfig(const AccessPointConfig &ap_config);

        void SetUpDnsOnDhcpServer(esp_netif_dns_info_t  dnsInfo);

        enum class State {
            NOT_INITIALIZED,
            INITIALIZED,
            CONFIGURED,
            STARTED,
            CLIENTS_CONNECTED,
            STOPPED,
            ERROR
        };

        void SetState(WifiAp::State state);

        bool EnableNat();

        bool DisableNat();

        WifiAp::State GetState() const;

    private:

        esp_netif_t *m_ap_netif;

        State m_State;

        static constexpr int DHCPS_OFFER_DNS = 0x02;

};


}
