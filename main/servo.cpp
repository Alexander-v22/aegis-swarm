
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "sdkconfig.h"




#define SERVO_PAN_GPIO 13
#define SERVO_MAX_US 2500
#define SERVO_MIN_US 1200

#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_RES LEDC_TIMER_16_BIT

#define SERVO_FREQ_HZ 50 
static const char* TAG = "SERVO";



// Need to setup for the duty cycle for PWM ledc timer
uint32_t angle_to_ledc_counts(uint8_t angle){
    uint32_t us = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US)* angle) / 180;
    return (us * (1 << 16)) / (1000000 /SERVO_FREQ_HZ); 
};

 void setup_ledc(void){
    ledc_timer_config_t timer = {}; 
        timer.duty_resolution = SERVO_RES;
        timer.freq_hz = SERVO_FREQ_HZ;
        timer.speed_mode = LEDC_LOW_SPEED_MODE; 
        timer.timer_num = SERVO_TIMER;
        timer.clk_cfg = LEDC_AUTO_CLK;

       ledc_timer_config(&timer);

    ledc_channel_config_t pan {};
        pan.channel = LEDC_CHANNEL_0; 
        pan.duty = 0;
        pan.gpio_num = SERVO_PAN_GPIO;
        pan.speed_mode = LEDC_LOW_SPEED_MODE;
        pan.hpoint = 0; 
        pan.timer_sel = SERVO_TIMER; 

        ledc_channel_config(&pan);

        ESP_LOGI(TAG, "INITIALIZED ON GPIO %d (CHANNEL %d)", 
                SERVO_PAN_GPIO, LEDC_CHANNEL_0);

}

void servo_update(uint8_t pan) {
    uint32_t duty = angle_to_ledc_counts(pan);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}