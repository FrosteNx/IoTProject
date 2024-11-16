// WifiManager.cpp
#include "WifiManager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include <netdb.h>        
#include <sys/socket.h>   
#include <iostream>
#include <string.h>

WifiManager::WifiManager(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass)
    : sta_ssid_(sta_ssid), sta_pass_(sta_pass), ap_ssid_(ap_ssid), ap_pass_(ap_pass),
      wifi_connected(false), ap_connected(false) {}

void WifiManager::init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_initiation));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::ip_event_handler, this));

    wifi_config_t sta_config = {};
    strcpy((char*)sta_config.sta.ssid, sta_ssid_);
    strcpy((char*)sta_config.sta.password, sta_pass_);

    wifi_config_t ap_config = {};
    strcpy((char*)ap_config.ap.ssid, ap_ssid_);
    strcpy((char*)ap_config.ap.password, ap_pass_);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    ESP_ERROR_CHECK(esp_wifi_start());
}

void WifiManager::start() {
    std::cout << "Starting Wi-Fi connection in AP+STA mode..." << std::endl;
}

bool WifiManager::is_wifi_connected() const { return wifi_connected; }
bool WifiManager::is_ap_connected() const { return ap_connected; }

void WifiManager::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    WifiManager *self = static_cast<WifiManager*>(arg);

    if (event_id == WIFI_EVENT_STA_START) {
        std::cout << "WIFI CONNECTING...." << std::endl;
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        std::cout << "WiFi CONNECTED in STA mode" << std::endl;
        self->wifi_connected = true;
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        self->wifi_connected = false;
        std::cout << "STA Mode Disconnected. Attempting to reconnect..." << std::endl;

        if (!self->ap_connected) {
            ESP_ERROR_CHECK(esp_wifi_connect());  // Reconnect only if no AP clients are connected
        }
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        std::cout << "Client connected to AP" << std::endl;
        self->ap_connected = true;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        std::cout << "Client disconnected from AP" << std::endl;
        self->ap_connected = false;

        if (!self->wifi_connected) {
            std::cout << "No clients on AP; reconnecting in STA mode..." << std::endl;
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
    }
}

void WifiManager::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    WifiManager *self = static_cast<WifiManager*>(arg);
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
        std::cout << "WiFi got IP: " << ip_str << std::endl;
        self->http_get_request();
    }
}

void WifiManager::http_get_request() {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    const char *server_url = "example.com";

    int err = getaddrinfo(server_url, "80", &hints, &res);
    if (err != 0 || res == NULL) {
        std::cerr << "Failed to resolve server address, error code: " << err << std::endl;
        return;
    }
    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        freeaddrinfo(res);
        return;
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        std::cerr << "Connection to server failed" << std::endl;
        close(sock);
        freeaddrinfo(res);
        return;
    }
    freeaddrinfo(res);

    const char *request = "GET / HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "User-Agent: ESP32Client/1.0\r\n"
                          "Connection: close\r\n\r\n";
    send(sock, request, strlen(request), 0);

    char buffer[1024];
    int received;
    while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        std::cout << buffer;
    }
    close(sock);
}