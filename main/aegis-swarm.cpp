
extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "driver/ledc.h"
    #include "driver/gpio.h"
    #include "esp_err.h"
    #include "esp_adc/adc_oneshot.h" //needed for the joystick
    #include "esp_log.h"

    #include "nvs_flash.h"
    #include "esp_netif.h"
    #include "esp_event.h"
    #include "esp_wifi.h"
    #include "esp_http_server.h"
    #include "esp_timer.h"
    #include "esp_mac.h"


}
#define SERVO_PAN_GPIO 13
#define SERVO_MAX_US 2500
#define SERVO_MIN_US 1200

#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_RES LEDC_TIMER_16_BIT

#define SERVO_FREQ_HZ 50 
static const char* TAG_SERVO = "SERVO";



// Need to setup for the duty cycle for PWM ledc timer
uint32_t angle_to_ledc_counts(uint8_t angle){
    uint32_t us = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US)* angle) / 180;
    return (us * (1 << 16)) / (1000000 /SERVO_FREQ_HZ); 
};

static void setup_ledc(void){
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

        ESP_LOGW(TAG_SERVO, "Starting up servo");

}

static void servo_update(uint8_t pan) {
    uint32_t duty = angle_to_ledc_counts(pan);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}


extern "C" void app_main(void) {
    setup_ledc();


    servo_update(90);
    vTaskDelay(pdMS_TO_TICKS(2000));
    while(1){

        //front sweeping
        for (int pan = 0; pan <=180 ; pan+=10){
            servo_update(pan);
            //giving the servo 0.2s (breathing room)
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // back sweeping
        for (int pan = 180; pan >= 0 ; pan -=10 ){
            servo_update(pan);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}