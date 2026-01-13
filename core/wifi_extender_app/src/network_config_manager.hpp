#pragma once

#include "wifi_extender_if/wifi_extender_config.hpp"
#include "data_storer_if/data_storer.hpp"

class NetworkConfigManager
{
    public:

        NetworkConfigManager();

        const WifiExtender::AccessPointConfig & GetApConfig();

        const WifiExtender::StaConfig & GetStaConfig();

        bool SetStaConfig(const WifiExtender::StaConfig & staConfig);

        bool SetApConfig(const WifiExtender::AccessPointConfig & staConfig);

    private:

        WifiExtender::AccessPointConfig m_ApConfig;
        WifiExtender::StaConfig m_StaConfig;

        static constexpr std::string_view m_ApConfigNvsKey  = "ApConfig";
        static constexpr std::string_view m_StaConfigNvsKey = "StaConfig";

        DataStorage::DataStorer & m_DataStorer;
};