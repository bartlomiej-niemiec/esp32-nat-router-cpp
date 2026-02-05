#include "wifi_nat_router_if/wifi_nat_router_config.hpp"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

namespace WifiNatRouter
{

AccessPointConfig::AccessPointConfig(std::string str_ssid, std::string str_password, uint32_t ipAddress, uint32_t networkmask, int max_clients):
    ssid{},
    password{},
    max_clients(max_clients),
    ipAddress(ipAddress),
    networkmask(networkmask)
{   
    const std::size_t max_ssid_len = MAX_SSID_SIZE - 1;
    const std::size_t n_ssid = std::min(str_ssid.size(), max_ssid_len);

    std::copy_n(str_ssid.begin(), n_ssid, ssid.begin());
    ssid[n_ssid] = '\0';  

    const std::size_t max_pass_len = MAX_PASSWORD_SIZE - 1;
    const std::size_t n_pass = std::min(str_password.size(), max_pass_len);

    std::copy_n(str_password.begin(), n_pass, password.begin());
    password[n_pass] = '\0'; 
}

AccessPointConfig::AccessPointConfig(std::string_view str_ssid, std::string_view str_password, uint32_t ipAddress, uint32_t networkmask, int max_clients):
    ssid{},
    password{},
    max_clients(max_clients),
    ipAddress(ipAddress),
    networkmask(networkmask)
{   
    const std::size_t max_ssid_len = MAX_SSID_SIZE - 1;
    const std::size_t n_ssid = std::min(str_ssid.size(), max_ssid_len);

    std::copy_n(str_ssid.begin(), n_ssid, ssid.begin());
    ssid[n_ssid] = '\0';  

    const std::size_t max_pass_len = MAX_PASSWORD_SIZE - 1;
    const std::size_t n_pass = std::min(str_password.size(), max_pass_len);

    std::copy_n(str_password.begin(), n_pass, password.begin());
    password[n_pass] = '\0'; 
}

    
AccessPointConfig::AccessPointConfig():
    ssid(),
    password(),
    max_clients()
{}

bool AccessPointConfig::IsValid() const {
    return ssid[0] != '\0' && password[0] != '\0';
}


StaConfig::StaConfig(std::string str_ssid, std::string str_password):
    ssid{},
    password{}
{
    const std::size_t max_ssid_len = MAX_SSID_SIZE - 1;
    const std::size_t n_ssid = std::min(str_ssid.size(), max_ssid_len);

    std::copy_n(str_ssid.begin(), n_ssid, ssid.begin());
    ssid[n_ssid] = '\0';  

    const std::size_t max_pass_len = MAX_PASSWORD_SIZE - 1;
    const std::size_t n_pass = std::min(str_password.size(), max_pass_len);

    std::copy_n(str_password.begin(), n_pass, password.begin());
    password[n_pass] = '\0'; 
}

StaConfig::StaConfig(std::string_view str_ssid, std::string_view str_password):
    ssid{},
    password{}
{
    const std::size_t max_ssid_len = MAX_SSID_SIZE - 1;
    const std::size_t n_ssid = std::min(str_ssid.size(), max_ssid_len);

    std::copy_n(str_ssid.begin(), n_ssid, ssid.begin());
    ssid[n_ssid] = '\0';  

    const std::size_t max_pass_len = MAX_PASSWORD_SIZE - 1;
    const std::size_t n_pass = std::min(str_password.size(), max_pass_len);

    std::copy_n(str_password.begin(), n_pass, password.begin());
    password[n_pass] = '\0'; 
}

StaConfig::StaConfig():
    ssid(),
    password()
{}

bool StaConfig::IsValid() const {
    return ssid[0] != '\0' && password[0] != '\0';
}


WifiNatRouterConfig::WifiNatRouterConfig(const AccessPointConfig& ap, const StaConfig& sta):
    apConfig(ap), staConfig(sta)
{}

WifiNatRouterConfig::WifiNatRouterConfig():
    apConfig(),
    staConfig()
{}

void WifiNatRouterConfig::printConfig(WifiNatRouterConfig& config)
{
    ESP_LOGI("WifiNatRouterConfig", "AP: %s", config.apConfig.ssid.data());
    ESP_LOGI("WifiNatRouterConfig", "STA: %s", config.staConfig.ssid.data());
}

bool WifiNatRouterConfig::operator==(const WifiNatRouterConfig& other) const {
    return apConfig == other.apConfig && staConfig == other.staConfig;
}

bool WifiNatRouterConfig::operator!=(const WifiNatRouterConfig& other) const {
    return !(*this == other);
}

void NatRouterStatistics::printStatistics(const ProtoStats& stats, const char * name)
{
    ESP_LOGI("NatRouterStatistics", "%s: xmit %llu", name, stats.xmit);
    ESP_LOGI("NatRouterStatistics", "%s: recv %llu", name, stats.recv);
    ESP_LOGI("NatRouterStatistics", "%s: fw %llu", name, stats.fw);
    ESP_LOGI("NatRouterStatistics", "%s: drop %llu", name, stats.drop);
    ESP_LOGI("NatRouterStatistics", "%s: chkerr %llu", name, stats.chkerr);
    ESP_LOGI("NatRouterStatistics", "%s: lenerr %llu", name, stats.lenerr);
}

void NatRouterStatistics::printNaptStatistics(const NaptStats& stats)
{
    ESP_LOGI("NatRouterStatistics", "NAPT: active tcp %llu", stats.nr_active_tcp);
    ESP_LOGI("NatRouterStatistics", "NAPT: active udp %llu", stats.nr_active_udp);
    ESP_LOGI("NatRouterStatistics", "NAPT: active icmp %llu", stats.nr_active_icmp);
    ESP_LOGI("NatRouterStatistics", "NAPT: active forced evictions %llu", stats.nr_forced_evictions);
}

};