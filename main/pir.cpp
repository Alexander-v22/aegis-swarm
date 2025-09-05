#include "pir.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"


//definign our LED and PIR pin and creating a timer using PIR_MIN_HIGH for detection 
#define LED_SIG GPIO_NUM_2
#define PIR_GPIO GPIO_NUM_14 
#define PIR_MIN_HIGH_MS 800

static const char *TAG = "PIR & LED";
static uint64_t motion_det_tmr = 0;   // 0 = not currently timing HIGH

esp_err_t pir_init(void) {
    gpio_config_t pir_config = {} ;
        pir_config.pin_bit_mask = 1ULL << PIR_GPIO;
        pir_config.mode = GPIO_MODE_INPUT;
        pir_config.pull_up_en = GPIO_PULLUP_DISABLE;
        pir_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        pir_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&pir_config);

    gpio_config_t led_config = {};
        led_config.pin_bit_mask = 1ULL << LED_SIG;
        led_config.mode = GPIO_MODE_OUTPUT; 
        led_config.mode = GPIO_MODE_OUTPUT;
        led_config.pull_up_en = GPIO_PULLUP_DISABLE;
        led_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        led_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&led_config);

        ESP_LOGI(TAG, "INITIALIZED");

        return ESP_OK;
}


bool pir_check(void){
    const int level = gpio_get_level(PIR_GPIO);
    const uint64_t start_time = esp_timer_get_time() / 1000ULL; // turning micro seconds into miliseconds

    //rising edge 
    if(level == 1 && motion_det_tmr == 0){
        motion_det_tmr = start_time; 
    }
    //falling edge
    if(level == 0 && motion_det_tmr > 0){
       const uint64_t duration = start_time - motion_det_tmr;
        motion_det_tmr = 0;

        if(duration >= PIR_MIN_HIGH_MS) {
            ESP_LOGI(TAG, "VALID HUMAN DETECTED");

            gpio_set_level(LED_SIG, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_SIG, 0);

            return true;
        } else {
            ESP_LOGI(TAG, "Ignored short spike");
        } 
    }
    return false; 
}

