#pragma once

#include <stdint.h>
#include <string>
#include "wifi_extender_config.hpp"
#include "wifi_extender_scanner_if.hpp"

namespace WifiExtender
{

class EventListener
{
    public:

        virtual ~EventListener() = default;

        virtual void Callback(WifiExtenderState event) = 0;

};

class WifiExtenderIf{

public:

    ~WifiExtenderIf() = default;

    virtual bool Startup(const WifiExtenderConfig & config) = 0;

    virtual bool RegisterListener(EventListener * pEventListener) = 0;

    virtual WifiExtenderState GetState() const = 0;

    virtual bool Shutdown() = 0;

    virtual bool TryToReconnect() = 0;

    virtual bool UpdateConfig(const WifiExtenderConfig & config) = 0;

    virtual WifiExtenderScannerIf * GetScanner() = 0;

    virtual int GetNoClients() const = 0;

};

}