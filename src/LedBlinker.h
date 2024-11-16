// LedBlinker.h

#ifndef LED_BLINKER_H
#define LED_BLINKER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class LedBlinker {
public:
    LedBlinker(gpio_num_t ledPin);
    void startBlinking();
    void stopBlinking();
    void init();

private:
    gpio_num_t led_pin;
    TaskHandle_t blinking_task_handle;
    bool blinking;
    static void blinkTask(void *pvParameter);
};

#endif // LED_BLINKER_H