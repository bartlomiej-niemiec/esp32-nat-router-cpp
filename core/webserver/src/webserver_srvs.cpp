#include "webserver_srvs.hpp"

#include "wifi_nat_router_if/wifi_nat_router_if.hpp"
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"

#include "config.hpp"

#include "esp_log.h"

#include <string>
#include <string_view>
#include <algorithm>


bool WebServerServices::m_ApNetworkConfigSaved{false};
bool WebServerServices::m_StaNetworkConfigSaved{false};
WifiNatRouterApp::WifiNatRouterAppIf * WebServerServices::m_pWifiNatRouterAppIf{nullptr};
WifiNatRouterApp::AppSnapshot WebServerServices::m_AppSnapshot{};
WifiNatRouterApp::AppSnapshot WebServerServices::m_PrevAppSnapshot{};
bool WebServerServices::m_RefreshRequired{false};


void WebServerServices::Init(WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterAppIf);
{
    assert(nullptr != pWifiNatRouterAppIf);
    if (m_pWifiNatRouterAppIf == nullptr)
    {
        m_pWifiNatRouterAppIf = pWifiNatRouterAppIf;
    }
}

int WebServerServices::AuthenticateUser(const char *user, const char *pass)
{
    if (!user || !pass) return 0;

    int userlevel = 0;
    std::vector<std::string> users = m_pUserCredentialManager->GetUserNames();
    std::string user_s(user);
    auto it = std::find(users.begin(), users.end(), user_s);
    if (it != users.end())
    {
        auto err = m_pUserCredentialManager->VerifyUserPassword(user, pass, userlevel);
        if (err.has_value())
        {
            userlevel = 0;
        }
    }

    return userlevel;
}


void WebServerServices::GetApSetting(saveapsettings * settings)
{
    static_assert(WifiNatRouter::WifiNatRouterHelpers::MAX_IP_ADDRESS_STRING_SIZE <= sizeof(settings->ipaddress));
    static_assert(WifiNatRouter::WifiNatRouterHelpers::MAX_IP_ADDRESS_STRING_SIZE <= sizeof(settings->networkmask));
    strncpy(settings->name, m_AppSnapshot.config.apConfig.ssid.data() ,sizeof(settings->name) - 1);
    WifiNatRouter::WifiNatRouterHelpers::ConvertU32ToIpAddressString(settings->ipaddress, m_AppSnapshot.config.apConfig.ipAddress);
    WifiNatRouter::WifiNatRouterHelpers::ConvertU32ToIpAddressString(settings->networkmask, m_AppSnapshot.config.apConfig.networkmask);
}

void WebServerServices::SetApSetting(saveapsettings * settings)
{
    if (m_AppSnapshot.configApplyInProgress) return;

    if (static_cast<uint8_t>(strnlen(settings->password, sizeof(settings->password))) < ESP_IDF_MINIMAL_PASSWORD_SIZE) return;

    std::string_view ssid(settings->name);
    std::string_view password(settings->password);

    uint32_t ipAddress;
    if (!WifiNatRouter::
WifiNatRouterHelpers::ConvertStringToIpAddress(settings->ipaddress, ipAddress)) return;

    uint32_t netMask;   
    if (!WifiNatRouter::
WifiNatRouterHelpers::ConvertStringToIpAddress(settings->networkmask, netMask)) return;

    WifiNatRouter::AccessPointConfig pendingApConfig(ssid, password, ipAddress, netMask);

    if (pendingApConfig != m_AppSnapshot.config.apConfig)
    {
        
        m_ApNetworkConfigSaved = true;
    }
}

void WebServerServices::GetLogin(login * loginData)
{
    if (!loginData) return;

    memset(loginData->username, 0, sizeof(loginData->username)); 
    memset(loginData->password, 0, sizeof(loginData->password)); 
}

void WebServerServices::SetLogin(login * loginData)
{
    (void) loginData;
}

void WebServerServices::GetStaSettings(savestasettings * settings)
{
    WifiNatRouter::
StaConfig staConfig = m_pNetworkConfigManager->GetStaConfig();
    strncpy(settings->Name, staConfig.ssid.data(), sizeof(settings->Name));
}

void WebServerServices::SetStaSetings(savestasettings * settings)
{
    if (m_ConfigChangeInProgress) return;

    static_assert(m_PendingStaConfig.password.max_size() >= sizeof(settings->Password));
    static_assert(m_PendingStaConfig.ssid.size() >= sizeof(settings->Name));
    
    if (settings->SSIDNoId > 0)
    {
        if (!m_ScannedNetworks.empty())
        {
            assert(settings->SSIDNoId <= 8);
            assert(m_PendingStaConfig.ssid.max_size() >= m_ScannedNetworks[settings->SSIDNoId - 1].ssid.size());
            std::string_view ssid(m_ScannedNetworks[settings->SSIDNoId - 1].ssid.data());
            std::string_view password(settings->Password);
            ESP_LOGI("WebServerServices", "STA: %s, password: %s", ssid.data(), password.data());
            m_PendingStaConfig = WifiNatRouter::
StaConfig(ssid, password);
        }
    }
    else
    {
        std::string_view ssid(settings->Name);
        std::string_view password(settings->Password);
        m_PendingStaConfig = WifiNatRouter::
StaConfig(ssid, password);
    }

    m_StaNetworkConfigSaved = true;
}   

void WebServerServices::GetStaScannedNetworks(stanetworks * networks)
{
    memset(networks, 0, sizeof(stanetworks));

    NetView nets[8] = {
        { networks->net1_ssid, &networks->net1_rssi, &networks->net1_channel, networks->net1_auth},
        { networks->net2_ssid, &networks->net2_rssi, &networks->net2_channel, networks->net2_auth},
        { networks->net3_ssid, &networks->net3_rssi, &networks->net3_channel, networks->net3_auth},
        { networks->net4_ssid, &networks->net4_rssi, &networks->net4_channel, networks->net4_auth},
        { networks->net5_ssid, &networks->net5_rssi, &networks->net5_channel, networks->net5_auth},
        { networks->net6_ssid, &networks->net6_rssi, &networks->net6_channel, networks->net6_auth},
        { networks->net7_ssid, &networks->net7_rssi, &networks->net7_channel, networks->net7_auth},
        { networks->net8_ssid, &networks->net8_rssi, &networks->net8_channel, networks->net8_auth}
    };

    if (!m_ScannedNetworks.empty())
    {
        auto noNetworksFound = std::min(m_ScannedNetworks.size(), size_t(8));
        networks->networkFound = noNetworksFound;

        for (size_t i = 0; i < noNetworksFound; i++) {
            const auto &w = m_ScannedNetworks[i];
            strncpy(nets[i].ssid, w.ssid.data(), sizeof(networks->net1_ssid) - 1);
            *nets[i].rssi    = w.rssi;
            *nets[i].channel = w.channel;
            strncpy(nets[i].auth,
                    WifiNatRouter::getAuthString(w.auth).data(), sizeof(networks->net1_auth) - 1);
        }
    
    }
}

void WebServerServices::StartStaScannningNetworks(struct mg_str body)
{
    if (m_ScanningRequested)
        return;

    m_ScanningRequested = true;
    m_pWifiNatRouter->GetScanner()->Scan();

    glue_update_state();
}

bool WebServerServices::IsStaScannningInProgress(void)
{
    return m_ScanningRequested.load();
}

void WebServerServices::WifiScannerCb(WifiNatRouter::
ScannerState state)
{
    if(state == WifiNatRouter::
ScannerState::Done)
    {
        m_ScannedNetworks.clear();
        auto scannedNetworks = m_pWifiNatRouter->GetScanner()->GetResults();
        for (const auto & network : scannedNetworks)
        {
            m_ScannedNetworks.emplace_back(
                WifiNatRouter::
WifiNetwork(
                    reinterpret_cast<const uint8_t*>(network.ssid.data()),
                    network.ssid.size(),
                    reinterpret_cast<const uint8_t*>(network.bssid.data()),
                    network.bssid.size(),
                    network.rssi,
                    network.channel,
                    network.auth
                )
            );
        }
        glue_update_state();
        m_ScanningRequested = false;
    }
}

void WebServerServices::GetWifiNatRouterInfo(info * info)
{
    WifiNatRouter::
StaConfig staConfig = m_pNetworkConfigManager->GetStaConfig();
    WifiNatRouter::
AccessPointConfig apConfig = m_pNetworkConfigManager->GetApConfig();
    
    strncpy(info->AP, apConfig.ssid.data() ,sizeof(info->AP) - 1);
    strncpy(info->STA, staConfig.ssid.data() ,sizeof(info->STA) - 1);

    WifiNatRouter::
WifiNatRouterState state = m_pWifiNatRouter->GetState();
    strncpy(info->State, WifiNatRouter::
WifiNatRouterHelpers::WifiNatRouterStaToString(state).data(), sizeof(info->State) - 1);

    info->Clients = m_pWifiNatRouter->GetNoClients();
    info->StaConn = state == WifiNatRouter::
WifiNatRouterState::RUNNING;
    info->StaIa = m_IsInternetAvailable;
}

void WebServerServices::SetWifiNatRouterInfo(info * info)
{

}
        
void WebServerServices::StartSaveEvent(mg_str params)
{

}

bool WebServerServices::IsSaveEventFinished()
{
    return true;
}

void WebServerServices::StartWifiNatRouterWithNewConfig(struct mg_str body)
{
    bool updateStaSettings = m_StaNetworkConfigSaved && m_pNetworkConfigManager->GetStaConfig() != m_PendingStaConfig;
    bool updateApSettings = m_ApNetworkConfigSaved && m_pNetworkConfigManager->GetApConfig() != m_PendingApConfig;

    WifiNatRouter::
WifiNatRouterConfig newConfig(
        updateApSettings ? m_PendingApConfig : m_pNetworkConfigManager->GetApConfig(),
        updateStaSettings ? m_PendingStaConfig : m_pNetworkConfigManager->GetStaConfig()
    );

    WifiNatRouter::
WifiNatRouterConfig::printConfig(newConfig);

    m_ConfigChangeInProgress = m_pWifiNatRouter->UpdateConfig(newConfig);

    if(!m_ConfigChangeInProgress) return;

    if (updateStaSettings)
    {
        m_pNetworkConfigManager->SetStaConfig(m_PendingStaConfig);
    }

    if (updateApSettings)
    {
        m_pNetworkConfigManager->SetApConfig(m_PendingApConfig);
    }
}
        
bool WebServerServices::IsApplyDisabled(void)
{
    WifiNatRouter::
WifiNatRouterState natRouterState = m_pWifiNatRouter->GetState();

    if ((natRouterState == WifiNatRouter::
WifiNatRouterState::STOPPING) ||
        (natRouterState == WifiNatRouter::
WifiNatRouterState::STOPPED)  ||
        (natRouterState == WifiNatRouter::
WifiNatRouterState::NEW_CONFIGURATION_PENDING) ||
        (natRouterState == WifiNatRouter::
WifiNatRouterState::STARTED)
    )
    {
        return true;
    }

    if (!m_StaNetworkConfigSaved && !m_ApNetworkConfigSaved) return true;

    bool isStaConfigHasChanged = false;
    if (m_StaNetworkConfigSaved)
    {
        const WifiNatRouter::
StaConfig & actualStaConfig = m_pNetworkConfigManager->GetStaConfig();
        isStaConfigHasChanged = m_PendingStaConfig != actualStaConfig;
    }

    bool isApConfigHasChanged = false;
    if (m_ApNetworkConfigSaved)
    {
        const WifiNatRouter::
AccessPointConfig & actualApConfig = m_pNetworkConfigManager->GetApConfig();
        isApConfigHasChanged = m_PendingApConfig != actualApConfig;
    }

    return !(isApConfigHasChanged || isStaConfigHasChanged);
}

void WebServerServices::Update()
{
    std::memcpy(&m_PrevAppSnapshot, &m_AppSnapshot, sizeof(WifiNatRouterApp::AppSnapshot));
    m_pWifiNatRouterAppIf->TryGetSnapshot(m_AppSnapshot);

    if (!m_RefreshRequired){
        m_RefreshRequired = memcmp(&m_PrevAppSnapshot, &m_AppSnapshot, sizeof(WifiNatRouterApp::AppSnapshot)) == 0;
    }
}

void WebServerServices::Refresh()
{
    if (m_RefreshRequired)
    {
        glue_update_state();
        m_RefreshRequired = false;
    }
}

