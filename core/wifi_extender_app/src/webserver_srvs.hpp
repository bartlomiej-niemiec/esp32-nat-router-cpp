#pragma once

#include "network_config_manager.hpp"
#include "user_credential_manager/user_credential_manager.hpp"
#include "wifi_extender_if/wifi_extender_if.hpp"
#include "wifi_event_monitor.hpp"

#include "mongoose/mongoose_glue.h"
#include <atomic>

class WebServerServices
{

    public:

        static void Init(UserCredential::UserCredentialManager * pUserCredentialManager,
                        WifiExtender::WifiExtenderIf * pWifiExtenderIf,
                        NetworkConfigManager * pNetworkConfigManager,
                        WifiEventMonitor * pWifiEventMonito);

        static int AuthenticateUser(const char *user, const char *pass);
        
        static void GetApSetting(saveapsettings * settings);

        static void SetApSetting(saveapsettings * settings);

        static void GetLogin(login * loginData);

        static void SetLogin(login * loginData);

        static void GetStaSettings(savestasettings * settings);

        static void SetStaSetings(savestasettings * settings);

        static void GetStaScannedNetworks(stanetworks * networks);

        static void StartStaScannningNetworks(struct mg_str body);
        
        static bool IsStaScannningInProgress(void);

        static void StartWifiExtenderWithNewConfig(struct mg_str body);
        
        static bool IsNewSavedAndDifferent(void);

        static void GetWifiExtenderInfo(info * info);

        static void SetWifiExtenderInfo(info * info);
        
        static void StartSaveEvent(mg_str params);

        static bool IsSaveEventFinished();
        

    private:

        static UserCredential::UserCredentialManager * m_pUserCredentialManager;

        static WifiExtender::WifiExtenderIf * m_pWifiExtender;

        static NetworkConfigManager * m_pNetworkConfigManager;

        static WifiExtender::AccessPointConfig m_PendingApConfig;

        static WifiExtender::StaConfig m_PendingStaConfig;

        static bool m_ApNetworkConfigSaved;
        static bool m_StaNetworkConfigSaved;

        static void WifiScannerCb(WifiExtender::ScannerState state);

        static void WifiEventCb(WifiExtender::WifiExtenderState event);
        static bool m_ConfigChangeInProgress;

        static std::atomic<bool> m_ScanningRequested;
        static std::vector<WifiExtender::WifiNetwork> m_ScannedNetworks;

        struct NetView {
            char *ssid;
            int  *rssi;
            int  *channel;
            char *auth;
            char *visibility;
        };
        
};