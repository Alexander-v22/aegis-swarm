#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "pir.h"

//    #include "esp_netif.h"
//    #include "esp_event.h"
//    #include "esp_wifi.h"
//   #include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_mac.h"


#include "servo.h"
#include "ultrasonic.h"
#include "pir.h"
   

static const char *TAG_OVERALL = "DATA";

void pir_task(void* arg){
    while(1){
        pir_check();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


extern "C" void app_main(void) {
    setup_ledc();
    ultrasonic_init();
    pir_init();

    vTaskDelay(pdMS_TO_TICKS(60000));
    servo_update(90);

    const int samples = 5;            
    const int delay_ms = 70; 

    while(1){
       xTaskCreate(pir_task, "pir_task", 2048, NULL, 5, NULL);
        pir_check();
        vTaskDelay(pdMS_TO_TICKS(20)); 

        //front sweeping
        for (int pan = 0; pan <=180 ; pan+=20){
           
            servo_update(pan);
            //giving the servo 0.2s (breathing room)
            vTaskDelay(pdMS_TO_TICKS(200));
            float dist_cm = hcsr_read_cm_med(samples, delay_ms);
            if(dist_cm>3.0f){
                ESP_LOGI(TAG_OVERALL, "pan=%3d°  distance=%.1f cm", pan, dist_cm);
            }
        }
        // back sweeping
        for (int pan = 180; pan >= 0 ; pan -=20 ){      
            servo_update(pan);
            vTaskDelay(pdMS_TO_TICKS(200));
             float dist_cm = hcsr_read_cm_med(samples, delay_ms);
            if(dist_cm>0.0f){
                ESP_LOGI(TAG_OVERALL, "pan=%3d°  distance=%.1f cm", pan, dist_cm);
            }
        }
    }
}