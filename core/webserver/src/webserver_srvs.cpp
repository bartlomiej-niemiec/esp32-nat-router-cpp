#include "webserver_srvs.hpp"

#include "wifi_nat_router_if/wifi_nat_router_if.hpp"
#include "wifi_nat_router_if/wifi_scanner_scanner_types.hpp"
#include "wifi_nat_router_if/wifi_nat_router_config.hpp"

#include "wifi_nat_router_app_config.hpp"

#include "esp_log.h"

#include <string>
#include <string_view>
#include <algorithm>

WifiNatRouterApp::WifiNatRouterAppIf * WebServerServices::m_pWifiNatRouterAppIf{nullptr};
UserCredential::UserCredentialManager * WebServerServices::m_pUserCredentialManager{nullptr};
WifiNatRouterApp::AppSnapshot WebServerServices::m_AppSnapshot{};
WifiNatRouterApp::AppSnapshot WebServerServices::m_PrevAppSnapshot{};
bool WebServerServices::m_RefreshRequired{false};


void WebServerServices::Init(WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterAppIf)
{
    assert(nullptr != pWifiNatRouterAppIf);
    if (m_pWifiNatRouterAppIf == nullptr)
    {
        m_pWifiNatRouterAppIf = pWifiNatRouterAppIf;
    }

    m_pUserCredentialManager = &UserCredential::UserCredentialManager::GetInstance();
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
        WifiNatRouterApp::Command cmd;
        cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdSetApConfig;
        cmd.apConfig = pendingApConfig;
        m_pWifiNatRouterAppIf->SendCommand(cmd);
    
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
    strncpy(settings->Name, m_AppSnapshot.config.staConfig.ssid.data(), sizeof(settings->Name));
}

void WebServerServices::SetStaSetings(savestasettings * settings)
{
    if (m_AppSnapshot.configApplyInProgress) return;

    if (settings->SSIDNoId > 0)
    {
        if (m_AppSnapshot.scannedCount > 0)
        {
            assert(settings->SSIDNoId <= 8);
            std::string_view ssid(m_AppSnapshot.scannedNetworks[settings->SSIDNoId - 1].ssid.data());
            std::string_view password(settings->Password);
            ESP_LOGI("WebServerServices", "STA: %s, password: %s", ssid.data(), password.data());
            WifiNatRouter::StaConfig newConfig(ssid, password);
            if (m_AppSnapshot.config.staConfig != newConfig)
            {
                WifiNatRouterApp::Command cmd;
                cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdSetStaConfig;
                cmd.staConfig = newConfig;
                m_pWifiNatRouterAppIf->SendCommand(cmd);
            
            }

        }
    }
    else
    {
        std::string_view ssid(settings->Name);
        std::string_view password(settings->Password);

        WifiNatRouter::StaConfig newConfig(ssid, password);

        if (m_AppSnapshot.config.staConfig != newConfig)
        {
            WifiNatRouterApp::Command cmd;
            cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdSetStaConfig;
            cmd.staConfig = newConfig;
            m_pWifiNatRouterAppIf->SendCommand(cmd);
        
        }
    }

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

    if (m_AppSnapshot.scannedCount > 0)
    {
        networks->networkFound = m_AppSnapshot.scannedCount;
        for (size_t i = 0; i < m_AppSnapshot.scannedCount; i++) {
            strncpy(nets[i].ssid, m_AppSnapshot.scannedNetworks[i].ssid.data(), sizeof(networks->net1_ssid) - 1);
            *nets[i].rssi    = m_AppSnapshot.scannedNetworks[i].rssi;
            *nets[i].channel = m_AppSnapshot.scannedNetworks[i].channel;
            strncpy(nets[i].auth,
                    WifiNatRouter::getAuthString(m_AppSnapshot.scannedNetworks[i].auth).data(), sizeof(networks->net1_auth) - 1);
        }
    
    }
}

void WebServerServices::StartStaScannningNetworks(struct mg_str body)
{
    if (m_AppSnapshot.scanState == WifiNatRouter::ScannerState::Scanning)
        return;

    WifiNatRouterApp::Command cmd;
    cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdStartScan;
    m_pWifiNatRouterAppIf->SendCommand(cmd);

}

bool WebServerServices::IsStaScannningInProgress(void)
{
    return m_AppSnapshot.scanState == WifiNatRouter::ScannerState::Scanning;
}

void WebServerServices::GetWifiNatRouterInfo(info * info)
{
    WifiNatRouter::StaConfig staConfig = m_AppSnapshot.config.staConfig;
    WifiNatRouter::AccessPointConfig apConfig = m_AppSnapshot.config.apConfig;
    
    strncpy(info->AP, apConfig.ssid.data() ,sizeof(info->AP) - 1);
    strncpy(info->STA, staConfig.ssid.data() ,sizeof(info->STA) - 1);

    WifiNatRouter::WifiNatRouterState state{m_AppSnapshot.routerState};
    strncpy(info->State, WifiNatRouter::WifiNatRouterHelpers::WifiNatRouterStaToString(state).data(), sizeof(info->State) - 1);

    info->Clients = m_AppSnapshot.noApClients;
    info->StaConn = state == WifiNatRouter::WifiNatRouterState::RUNNING;
    info->StaIa = m_AppSnapshot.internetAccess;
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
    if (m_AppSnapshot.configApplyInProgress) return;

    if (m_AppSnapshot.readyForApplyingConfig)
    {
        WifiNatRouterApp::Command cmd;
        cmd.cmd = WifiNatRouterApp::WifiNatRouterCmd::CmdApplyNetConfig;
        m_pWifiNatRouterAppIf->SendCommand(cmd);
    
    }

}
        
bool WebServerServices::IsApplyDisabled(void)
{
    if ((m_AppSnapshot.routerState == WifiNatRouter::
WifiNatRouterState::STOPPING) ||
        (m_AppSnapshot.routerState == WifiNatRouter::
WifiNatRouterState::STOPPED)  ||
        (m_AppSnapshot.routerState == WifiNatRouter::
WifiNatRouterState::NEW_CONFIGURATION_PENDING) ||
        (m_AppSnapshot.routerState == WifiNatRouter::
WifiNatRouterState::STARTED)
    )
    {
        return true;
    }

    if (m_AppSnapshot.configApplyInProgress) return true;

    return !m_AppSnapshot.readyForApplyingConfig;
}

void WebServerServices::Update()
{
    if (m_pWifiNatRouterAppIf == nullptr) {
        return;
    }

    m_PrevAppSnapshot = m_AppSnapshot;

    if (!m_pWifiNatRouterAppIf->TryGetSnapshot(m_AppSnapshot)) return;

    if (!m_RefreshRequired){
        m_RefreshRequired = !(m_PrevAppSnapshot == m_AppSnapshot);
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
