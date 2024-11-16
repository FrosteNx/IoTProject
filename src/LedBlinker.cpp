// LedBlinker.cpp

#include "LedBlinker.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

extern bool wifi_connected;
extern bool ap_connected;

LedBlinker::LedBlinker(gpio_num_t ledPin) : led_pin(ledPin), blinking_task_handle(nullptr), blinking(false) {}

void LedBlinker::startBlinking() {
    if (!blinking) {
        blinking = true;
        xTaskCreate(&LedBlinker::blinkTask, "led_blink_task", 2048, this, 5, &blinking_task_handle);
    }
}

void LedBlinker::stopBlinking() {
    if (blinking) {
        vTaskDelete(blinking_task_handle);
        gpio_set_level(led_pin, 1); 
        blinking = false;
    }
}

void LedBlinker::init() {
    gpio_reset_pin(led_pin);
    gpio_set_direction(led_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(led_pin, 1);
}

void LedBlinker::blinkTask(void *pvParameter) {
    LedBlinker *blinker = static_cast<LedBlinker*>(pvParameter);
    while (!wifi_connected && !ap_connected) {
        gpio_set_level(blinker->led_pin, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(blinker->led_pin, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}