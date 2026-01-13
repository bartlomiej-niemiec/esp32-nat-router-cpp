#include "wifi_ap.hpp"
#include "assert.h"
#include <utility>
#include <string.h>
#include <cstring>

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/lwip_napt.h"

#include "esp_wifi.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"

#include "esp_log.h"

namespace WifiExtender
{

WifiAp::WifiAp():
    m_ap_netif(nullptr),
    m_State(WifiAp::State::NOT_INITIALIZED)
{
};

WifiAp::~WifiAp()
{
    esp_netif_destroy_default_wifi(m_ap_netif);
}

bool WifiAp::Init()
{
    m_ap_netif = esp_netif_create_default_wifi_ap();
    assert(nullptr != m_ap_netif);

    return true;
}


bool WifiAp::SetConfig(const AccessPointConfig &ap_config)
{
    wifi_config_t ap_cfg = {};
    ap_cfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;

    size_t ssid_size = strnlen(ap_config.ssid.data(), AccessPointConfig::MAX_SSID_SIZE);
    if (ssid_size < sizeof(ap_cfg.ap.ssid)) {
        memcpy(ap_cfg.ap.ssid, ap_config.ssid.data(), ssid_size + 1);
        ap_cfg.ap.ssid_len = ssid_size;
    } else {
        memcpy(ap_cfg.ap.ssid, ap_config.ssid.data(), sizeof(ap_cfg.ap.ssid));
        ap_cfg.ap.ssid[sizeof(ap_cfg.ap.ssid) - 1] = '\0';
        ap_cfg.ap.ssid_len = strlen((char*)ap_cfg.ap.ssid);
    }

    size_t password_size = strnlen(ap_config.password.data(), AccessPointConfig::MAX_PASSWORD_SIZE);
    if (password_size < sizeof(ap_cfg.ap.password)) {
        memcpy(ap_cfg.ap.password, ap_config.password.data(), password_size + 1);
    } else {
        memcpy(ap_cfg.ap.password, ap_config.password.data(), sizeof(ap_cfg.ap.password));
        ap_cfg.ap.password[sizeof(ap_cfg.ap.password) - 1] = '\0';
    }

    esp_netif_ip_info_t ip_info {
        .ip = ap_config.ipAddress,
        .netmask = ap_config.networkmask,
        .gw = ap_config.ipAddress,
    };

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(m_ap_netif));          
    ESP_ERROR_CHECK(esp_netif_set_ip_info(m_ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(m_ap_netif));         

    esp_err_t result = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    assert(result == ESP_OK);
    return true;
}

void WifiAp::SetUpDnsOnDhcpServer(esp_netif_dns_info_t  dnsInfo)
{
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(m_ap_netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(m_ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(m_ap_netif, ESP_NETIF_DNS_MAIN, &dnsInfo));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(m_ap_netif));
}

void WifiAp::SetState(WifiAp::State state)
{
    m_State = state;
}

WifiAp::State WifiAp::GetState() const
{
    return m_State;
}

bool WifiAp::EnableNat()
{
    if (esp_netif_napt_enable(m_ap_netif) != ESP_OK)
    {
        ESP_LOGE("WIFI_STA", "NAPT not enabled on the netif: %p", m_ap_netif);
        return false;
    }
    return true;
}

bool WifiAp::DisableNat()
{
    return esp_netif_napt_disable(m_ap_netif) == ESP_OK ? true : false;
}


}

