#include "wifi_scanner.hpp"

namespace WifiExtender
{

WifiScanner::WifiScanner():
    m_ScannedNetworks(),
    m_State(ScannerState::Idle)
{
}

bool WifiScanner::StartScanFor(const ScanOptions& opts)
{
    esp_err_t error = ESP_FAIL;

    if (opts.passive)
    {
        error = StartPassiveScan(opts);
    }
    else
    {
        error = StartActiveScan(opts);
    }

    if (error == ESP_OK)
    {
        m_ScannedNetworks.clear();
        m_State = ScannerState::Scanning;
    }

    return  error == ESP_OK;
}

esp_err_t WifiScanner::StartPassiveScan(const ScanOptions& opts)
{
    wifi_scan_config_t scanConfig = {};
    wifi_scan_time_t scanTime = {};                    
    scanTime.passive = SCAN_TIME_PER_CHANNEL_PASSIVE_MS;

    scanConfig.scan_time = scanTime;
    scanConfig.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    scanConfig.show_hidden = opts.show_hidden;
    scanConfig.home_chan_dwell_time = opts.dwell_ms;

    return esp_wifi_scan_start(&scanConfig, false);
}

esp_err_t WifiScanner::StartActiveScan(const ScanOptions& opts)
{
    wifi_scan_config_t scanConfig = {};
    wifi_scan_time_t scanTime = {};    
    scanTime.active = {
        .min = 100,
        .max = 150
    }; ;
    scanConfig.scan_time = scanTime;
    scanConfig.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scanConfig.show_hidden = opts.show_hidden;
    scanConfig.home_chan_dwell_time = opts.dwell_ms;

    return esp_wifi_scan_start(&scanConfig, false);
}

bool WifiScanner::CancelScan()
{
    m_State = ScannerState::Cancelled;
    return esp_wifi_scan_stop() == ESP_OK;
}

void WifiScanner::CleanResults()
{
    esp_wifi_clear_ap_list();
}

void WifiScanner::ScanningCompleteSignal()
{
    uint16_t count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));
    if (count == 0) { m_State = ScannerState::Done; return; }

    std::vector<wifi_ap_record_t> recs(count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, recs.data()));

    m_ScannedNetworks.clear();
    m_ScannedNetworks.reserve(count);
    for (auto& r : recs) {
        size_t ssid_len = strnlen(reinterpret_cast<const char*>(r.ssid), sizeof(r.ssid));
        m_ScannedNetworks.emplace_back(
            WifiNetwork(
                r.ssid,
                ssid_len,
                r.bssid,
                sizeof(r.bssid),
                r.rssi,
                r.primary,
                ToAuthMode(r.authmode)
            )
        );
    }
    std::sort(m_ScannedNetworks.begin(), m_ScannedNetworks.end(), [](const auto & n1, const auto & n2){return n1.rssi > n2.rssi;});
    m_State = m_State == ScannerState::Cancelled ? ScannerState::Idle : ScannerState::Done;
}

const std::vector<WifiNetwork> & WifiScanner::GetResults() const
{
    return m_ScannedNetworks;
}

ScannerState WifiScanner::GetScannerState() const
{
    return m_State;
}

AuthMode WifiScanner::ToAuthMode(wifi_auth_mode_t mode)
{
    switch (mode)
    {
        case WIFI_AUTH_OPEN: return AuthMode::Open;
        case WIFI_AUTH_WEP: return AuthMode::WEP;
        case WIFI_AUTH_WPA_PSK: return AuthMode::WPA_PSK;
        case WIFI_AUTH_WPA2_PSK: return AuthMode::WPA2_PSK;
        case WIFI_AUTH_WPA_WPA2_PSK: return AuthMode::WPA_WPA2_PSK;
        case WIFI_AUTH_WPA3_PSK: return AuthMode::WPA3_PSK;
        default: return AuthMode::Unknown;
    };
}

}
