#include "internet_activity_monitor.hpp"
#include "esp_task.h"

InternetActivityMonitor::InternetActivityMonitor(WifiNatRouterApp::WifiNatRouterAppEventQueue & rEventQueue):
    m_rEventQueue(rEventQueue),
    m_runnning(false),
    m_ping_timeout_count(0),
    m_ping_success_count(0)

{
    esp_ping_callbacks_t cbs{
        .cb_args = this,
        .on_ping_success = on_ping_success_cb,
        .on_ping_timeout = on_ping_timeout_cb,
        .on_ping_end = on_ping_end_cb
    };

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = IPADDR4_INIT_BYTES(8,8,8,8);
    
    ESP_ERROR_CHECK(esp_ping_new_session(
        &config,
        &cbs,
        &m_InternetAccessPingHandle
    ));
}

InternetActivityMonitor::~InternetActivityMonitor()
{
    if(m_runnning)
    {
        esp_ping_stop(m_InternetAccessPingHandle);
    }
    esp_ping_delete_session(m_InternetAccessPingHandle);
}

bool InternetActivityMonitor::Check()
{
    if (m_runnning) return true;

    m_ping_timeout_count = 0;
    m_ping_success_count = 0;
    m_runnning = esp_ping_start(m_InternetAccessPingHandle) == ESP_OK;
    return m_runnning;
}