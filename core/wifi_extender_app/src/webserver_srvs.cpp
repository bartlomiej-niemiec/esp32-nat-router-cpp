#include "webserver_srvs.hpp"

#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_extender_if/wifi_extender_config.hpp"

#include <string_view>
#include <algorithm>

UserCredential::UserCredentialManager * WebServerServices::m_pUserCredentialManager = nullptr;
WifiExtender::WifiExtenderIf * WebServerServices::m_pWifiExtender = nullptr;
NetworkConfigManager * WebServerServices::m_pNetworkConfigManager = nullptr;
std::atomic<bool> WebServerServices::m_ScanningRequested = false;
std::vector<WifiExtender::WifiNetwork> WebServerServices::m_ScannedNetworks{};
WifiExtender::AccessPointConfig WebServerServices::m_PendingApConfig{};
WifiExtender::StaConfig WebServerServices::m_PendingStaConfig{};
bool WebServerServices::m_ApNetworkConfigSaved = false;
bool WebServerServices::m_StaNetworkConfigSaved = false;
bool WebServerServices::m_ConfigChangeInProgress = false;

void WebServerServices::Init(
                        UserCredential::UserCredentialManager * pUserCredentialManager,
                        WifiExtender::WifiExtenderIf * pWifiExtenderIf,
                        NetworkConfigManager * pNetworkConfigManager,
                        WifiEventMonitor * pWifiEventMonitor
                    )
{
    assert(nullptr != pUserCredentialManager);
    if (m_pUserCredentialManager == nullptr)
    {
        m_pUserCredentialManager = pUserCredentialManager;
    }

    assert(nullptr != pWifiExtenderIf);
    if (m_pWifiExtender == nullptr)
    {
        m_pWifiExtender = pWifiExtenderIf;
        m_pWifiExtender->GetScanner()->RegisterStateListener(WifiScannerCb);
    }

    assert(nullptr != pNetworkConfigManager);
    if (m_pNetworkConfigManager == nullptr)
    {
        m_pNetworkConfigManager = pNetworkConfigManager;
    }

    assert(nullptr != pWifiEventMonitor);
    pWifiEventMonitor->Subscribe(WifiEventCb);
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
    WifiExtender::AccessPointConfig apConfig = m_pNetworkConfigManager->GetApConfig();
    strncpy(settings->name, apConfig.ssid.data() ,sizeof(settings->name));
}

void WebServerServices::SetApSetting(saveapsettings * settings)
{
    if (m_ConfigChangeInProgress) return;

    assert(m_PendingApConfig.ssid.max_size() >= sizeof(settings->name));
    std::string_view ssid(settings->name);

    assert(m_PendingApConfig.password.max_size() >= sizeof(settings->password));
    std::string_view password(settings->password);

    uint32_t ipAddress;
    if (!WifiExtender::WifiExtenderHelpers::ConvertStringToIpAddress(settings->ipaddress, ipAddress)) return;

    m_PendingApConfig = WifiExtender::AccessPointConfig(ssid, password, ipAddress, 0xFFFFFFFF);

    m_ApNetworkConfigSaved = true;
}

void WebServerServices::GetLogin(login * loginData)
{

}

void WebServerServices::SetLogin(login * loginData)
{

}

void WebServerServices::GetStaSettings(savestasettings * settings)
{
    WifiExtender::StaConfig staConfig = m_pNetworkConfigManager->GetStaConfig();
    strncpy(settings->Name, staConfig.ssid.data() ,sizeof(settings->Name));
}

void WebServerServices::SetStaSetings(savestasettings * settings)
{
    if (m_ConfigChangeInProgress) return;

    assert(m_PendingStaConfig.password.max_size() >= sizeof(settings->Password));
    if (settings->SSIDNoId > 0)
    {
        if (!m_ScannedNetworks.empty())
        {
            assert(settings->SSIDNoId <= 5);
            assert(m_PendingStaConfig.ssid.max_size() >= m_ScannedNetworks[settings->SSIDNoId - 1].ssid.size());
            std::string_view ssid(m_ScannedNetworks[settings->SSIDNoId - 1].ssid.data());
            std::string_view password(settings->Password);
            ESP_LOGI("WebServerServices", "STA: %s, password: %s", ssid.data(), password.data());
            m_PendingStaConfig = WifiExtender::StaConfig(ssid, password);
        }
    }
    else
    {
        assert(m_PendingStaConfig.ssid.size() >= sizeof(settings->Name));
        std::string_view ssid(settings->Name);
        std::string_view password(settings->Password);
        m_PendingStaConfig = WifiExtender::StaConfig(ssid, password);
    }

    m_StaNetworkConfigSaved = true;
}   

void WebServerServices::GetStaScannedNetworks(stanetworks * networks)
{
    memset(networks, 0, sizeof(stanetworks));

    NetView nets[5] = {
        { networks->net1_ssid, &networks->net1_rssi, &networks->net1_channel, networks->net1_auth, networks->net1_visibility },
        { networks->net2_ssid, &networks->net2_rssi, &networks->net2_channel, networks->net2_auth, networks->net2_visibility },
        { networks->net3_ssid, &networks->net3_rssi, &networks->net3_channel, networks->net3_auth, networks->net3_visibility },
        { networks->net4_ssid, &networks->net4_rssi, &networks->net4_channel, networks->net4_auth, networks->net4_visibility },
        { networks->net5_ssid, &networks->net5_rssi, &networks->net5_channel, networks->net5_auth, networks->net5_visibility }
    };
    if (!m_ScannedNetworks.empty())
    {
        auto noNetworksFound = std::min(m_ScannedNetworks.size(), size_t(5));

        for (size_t i = 0; i < noNetworksFound; i++) {
            const auto &w = m_ScannedNetworks[i];
            strncpy(nets[i].ssid, w.ssid.data(), 31);
            *nets[i].rssi    = w.rssi;
            *nets[i].channel = w.channel;
            strncpy(nets[i].auth,
                    WifiExtender::getAuthString(w.auth).data(), 31);
        }
        
        /// Clear rest networks
        for (int i = noNetworksFound; i < 5; i++)
        {
            const auto &w = m_ScannedNetworks[i];
            memset(nets[i].ssid, 0, 31);
            *nets[i].rssi    = 0;
            *nets[i].channel = 0;
            memset(nets[i].auth, 0, 31);
        }
    }
}

void WebServerServices::StartStaScannningNetworks(struct mg_str body)
{
    if (m_ScanningRequested)
        return;

    m_ScanningRequested = true;
    m_pWifiExtender->GetScanner()->Scan();

    glue_update_state();
}

bool WebServerServices::IsStaScannningInProgress(void)
{
    return m_ScanningRequested.load();
}

void WebServerServices::WifiScannerCb(WifiExtender::ScannerState state)
{
    if(state == WifiExtender::ScannerState::Done)
    {
        m_ScannedNetworks.clear();
        auto scannedNetworks = m_pWifiExtender->GetScanner()->GetResults();
        for (const auto & network : scannedNetworks)
        {
            m_ScannedNetworks.emplace_back(
                WifiExtender::WifiNetwork(
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

void WebServerServices::GetWifiExtenderInfo(info * info)
{
    WifiExtender::StaConfig staConfig = m_pNetworkConfigManager->GetStaConfig();
    WifiExtender::AccessPointConfig apConfig = m_pNetworkConfigManager->GetApConfig();
    
    strncpy(info->AP, apConfig.ssid.data() ,sizeof(info->AP));
    strncpy(info->STA, staConfig.ssid.data() ,sizeof(info->STA));

    WifiExtender::WifiExtenderState state = m_pWifiExtender->GetState();
    strncpy(info->State, WifiExtender::WifiExtenderHelpers::WifiExtenderStaToString(state).data(), sizeof(info->State));

    info->Clients = m_pWifiExtender->GetNoClients();
    info->StaConn = state == WifiExtender::WifiExtenderState::RUNNING;
}

void WebServerServices::SetWifiExtenderInfo(info * info)
{

}
        
void WebServerServices::StartSaveEvent(mg_str params)
{

}

bool WebServerServices::IsSaveEventFinished()
{
    return true;
}

void WebServerServices::StartWifiExtenderWithNewConfig(struct mg_str body)
{
    bool updateStaSettings = m_StaNetworkConfigSaved && m_pNetworkConfigManager->GetStaConfig() != m_PendingStaConfig;
    bool updateApSettings = m_ApNetworkConfigSaved && m_pNetworkConfigManager->GetApConfig() != m_PendingApConfig;

    WifiExtender::WifiExtenderConfig newConfig(
        updateApSettings ? m_PendingApConfig : m_pNetworkConfigManager->GetApConfig(),
        updateStaSettings ? m_PendingStaConfig : m_pNetworkConfigManager->GetStaConfig()
    );

    WifiExtender::WifiExtenderConfig::printConfig(newConfig);

    m_ConfigChangeInProgress = m_pWifiExtender->UpdateConfig(newConfig);

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
        
bool WebServerServices::IsNewSavedAndDifferent(void)
{
    WifiExtender::WifiExtenderState extenderState = m_pWifiExtender->GetState();

    if ((extenderState == WifiExtender::WifiExtenderState::STOPPING) ||
        (extenderState == WifiExtender::WifiExtenderState::STOPPED)  ||
        (extenderState == WifiExtender::WifiExtenderState::NEW_CONFIGURATION_PENDING) ||
        (extenderState == WifiExtender::WifiExtenderState::STARTED)
    )
    {
        return true;
    }

    if (!m_StaNetworkConfigSaved && !m_ApNetworkConfigSaved) return true;

    bool isStaConfigHasChanged = false;
    if (m_StaNetworkConfigSaved)
    {
        const WifiExtender::StaConfig & actualStaConfig = m_pNetworkConfigManager->GetStaConfig();
        isStaConfigHasChanged = m_PendingStaConfig != actualStaConfig;
    }

    bool isApConfigHasChanged = false;
    if (m_ApNetworkConfigSaved)
    {
        const WifiExtender::AccessPointConfig & actualApConfig = m_pNetworkConfigManager->GetApConfig();
        isApConfigHasChanged = m_PendingApConfig != actualApConfig;
    }

    return !(isApConfigHasChanged || isStaConfigHasChanged);
}

void WebServerServices::WifiEventCb(WifiExtender::WifiExtenderState event)
{
    if (m_ConfigChangeInProgress)
    {
        if (event == WifiExtender::WifiExtenderState::CONNECTING)
        {
            m_ConfigChangeInProgress = false;
            m_StaNetworkConfigSaved = false;
            m_ApNetworkConfigSaved = false;
            glue_update_state();
        }
    }
}