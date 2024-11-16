// WifiManager.h
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_event.h"
#include <string>

class WifiManager {
public:
    WifiManager(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass);

    void init();
    void start();
    void http_get_request();

    bool is_wifi_connected() const;
    bool is_ap_connected() const;

private:
    const char* sta_ssid_;
    const char* sta_pass_;
    const char* ap_ssid_;
    const char* ap_pass_;
    bool wifi_connected;
    bool ap_connected;

    static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};

#endif // WIFI_MANAGER_H