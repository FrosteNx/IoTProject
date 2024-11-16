#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <iostream>
#include <string.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <netdb.h>

#define LOG_TAG "MAIN"

const char *ssid = "hotspot123";
const char *pass = "321hotspot";
const char *ap_ssid = "ESP32_AP";       
const char *ap_pass = "12345678";       

bool wifi_connected = false;
bool ap_connected = false;    
bool blinking_task_running = false;

#define LED_PIN GPIO_NUM_2

TaskHandle_t led_blink_task_handle = NULL;

void led_blink_task(void *pvParameter) {
    while (!wifi_connected && !ap_connected) {   
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void start_led_blink() {
    if (!blinking_task_running) {
        blinking_task_running = true;
        xTaskCreate(&led_blink_task, "led_blink_task", 2048, NULL, 5, &led_blink_task_handle);
    }
}

void stop_led_blink() {
    if (blinking_task_running) {
        vTaskDelete(led_blink_task_handle);
        gpio_set_level(LED_PIN, 1); 
        blinking_task_running = false;
    }
}

extern "C" {
    static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_id == WIFI_EVENT_STA_START) {
            std::cout << "WIFI CONNECTING...." << std::endl;
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
            std::cout << "WiFi CONNECTED in STA mode" << std::endl;
            wifi_connected = true;     
            stop_led_blink();
        } else if (event_id == WIFI_EVENT_AP_START) {
            std::cout << "AP Mode Enabled" << std::endl;
            wifi_connected = true;     
            stop_led_blink();
        } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {  
            std::cout << "Client connected to AP" << std::endl;
            ap_connected = true;      
            stop_led_blink();
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) { 
            std::cout << "Client disconnected from AP" << std::endl;
            ap_connected = false;     
            
            if (!wifi_connected) {    
                std::cout << "Reconnecting in STA mode..." << std::endl;
                ESP_ERROR_CHECK(esp_wifi_connect());
            } else {
                start_led_blink();
            }
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            std::cout << "WiFi lost connection" << std::endl;

            wifi_connected = false;    

            if (!ap_connected) {      
                std::cout << "Reconnecting in STA mode..." << std::endl;
                ESP_ERROR_CHECK(esp_wifi_connect());
                start_led_blink();
            } else {
                std::cout << "AP client still connected, skipping STA reconnection." << std::endl;
            }
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            std::cout << "AP Mode Disabled" << std::endl;
            wifi_connected = false;    
            start_led_blink();
        }
    }

    void http_get_request() {
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
        std::cout << "Address resolved, proceeding with socket creation..." << std::endl;

        int sock = socket(res->ai_family, res->ai_socktype, 0);
        if (sock < 0) {
            std::cerr << "Socket creation failed" << std::endl;
            freeaddrinfo(res);
            return;
        }
        std::cout << "Socket created successfully..." << std::endl;

        if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
            std::cerr << "Connection to server failed" << std::endl;
            close(sock);
            freeaddrinfo(res);
            return;
        }
        std::cout << "Connected to server..." << std::endl;

        freeaddrinfo(res);

        const char *request = "GET / HTTP/1.1\r\n"
                            "Host: example.com\r\n"
                            "User-Agent: ESP32Client/1.0\r\n"
                            "Connection: close\r\n\r\n";

        if (send(sock, request, strlen(request), 0) < 0) {
            std::cerr << "Sending GET request failed" << std::endl;
            close(sock);
            return;
        }
        std::cout << "GET request sent successfully, awaiting response..." << std::endl;

        char buffer[1024];
        int received;
        while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[received] = '\0';
            std::cout << buffer;
        }

        if (received < 0) {
            std::cerr << "Receiving data failed" << std::endl;
        } else {
            std::cout << "\nResponse received successfully." << std::endl;
        }

        close(sock);
    }

    static void ip_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            char ip_str[16];
            esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
            std::cout << "WiFi got IP: " << ip_str << std::endl;

            http_get_request();
        }
    }

    void wifi_connection() {
        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        esp_netif_create_default_wifi_sta();  
        esp_netif_create_default_wifi_ap();  

        wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wifi_initiation));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

        wifi_config_t sta_config = {};
        strcpy((char*)sta_config.sta.ssid, ssid);
        strcpy((char*)sta_config.sta.password, pass);

        wifi_config_t ap_config = {};
        strcpy((char*)ap_config.ap.ssid, ap_ssid);
        strcpy((char*)ap_config.ap.password, ap_pass);
        ap_config.ap.channel = 1;
        ap_config.ap.max_connection = 3;
        ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

        ESP_ERROR_CHECK(esp_wifi_start());

        std::cout << "WiFi initialized in AP+STA mode with SSID: " << ssid << " and PASSWORD: " << pass << std::endl;
    }

    void led_init() {
        gpio_reset_pin(LED_PIN);
        gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(LED_PIN, 1);
    }

    void app_main() {
        ESP_LOGI(LOG_TAG, "Initializing NVS");
        ESP_ERROR_CHECK(nvs_flash_init());

        std::cout << "Starting Wi-Fi connection..." << std::endl;

        led_init();
        wifi_connection();

        while (!wifi_connected) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}
