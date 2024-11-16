#pragma once

#include "esp_wifi.h"
#include <esp_err.h>

namespace WIFI
{
class Wifi
{
public:
    enum class state_e
    {
        NOT_INITIALIZED,
        INITIALIZED,
        WAITING_FOR_CREDENTIALS,
        READY_TO_CONNECT,
        CONNECTING,
        WAITING_FOR_IP,
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    esp_err_t init(void); // Setup
    esp_err_t begin(void); // Start Wifi connection

    state_e get_state(void);
    const char* get_mac(void);

private:
    void state_machine(void);
};
} // namespace WIFI