#pragma once

#include "user_credential_manager/user_credential_manager.hpp"
#include "wifi_nat_router_if/wifi_nat_router_if.hpp"
#include "wifi_nat_router_app_if.hpp"
#include "mongoose/mongoose_glue.h"

#include <atomic>
#include <memory>

class WebServerServices
{

    public:

        static void Init(WifiNatRouterApp::WifiNatRouterAppIf * pWifiNatRouterAppIf);

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

        static void StartWifiNatRouterWithNewConfig(struct mg_str body);
        
        static bool IsApplyDisabled(void);

        static void GetWifiNatRouterInfo(info * info);

        static void SetWifiNatRouterInfo(info * info);
        
        static void StartSaveEvent(mg_str params);

        static bool IsSaveEventFinished();

        static void GetIpProtoStas(struct proto_stats_ip *data);

        static void GetTcpProtoStas(struct proto_stats_udp *data);

        static void GetUdpProtoStas(struct proto_stats_tcp *data);

        static void GetIcmpProtoStas(struct proto_stats_icmp *data);

        static void GetNaptProtoStas(struct napt_stats *data);

        static void Update();

        static void Refresh();

    private:

        static WifiNatRouterApp::WifiNatRouterAppIf * m_pWifiNatRouterAppIf;
        static UserCredential::UserCredentialManager * m_pUserCredentialManager;

        static WifiNatRouterApp::AppSnapshot m_PrevAppSnapshot;
        static WifiNatRouterApp::AppSnapshot m_AppSnapshot;
        static bool m_RefreshRequired;

        struct NetView {
            char *ssid;
            int  *rssi;
            int  *channel;
            char *auth;
        };
        

};
