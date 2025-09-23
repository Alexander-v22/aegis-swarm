#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "pir.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "nvs_flash.h"


#include "servo.h"
#include "ultrasonic.h"
#include "pir.h"
#include "swarm.h"

static const char *TAG_OVERALL = "DATA";

void pir_task(void* arg){
    while(1){
        pir_check();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

struct HeartbeatArgs {
    uint32_t botA;
    uint32_t botB;
};

static void hb_task(void* arg) {
    auto* a = static_cast<HeartbeatArgs*>(arg);
    const uint8_t pir_dummy = 0;
    for (;;) {
        if (a->botA) swarm_publish_telem_to(a->botA, pir_dummy, 0.0f);
        if (a->botB) swarm_publish_telem_to(a->botB, pir_dummy, 0.0f);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {

    // 1) NVS first
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }


    setup_ledc();
    ultrasonic_init();
    pir_init();

    swarm_init(1);

    vTaskDelay(pdMS_TO_TICKS(60000));// PIR warm up 

    uint32_t botA = swarm_add_peer("6C:C8:40:8A:98:48"); // first Aegis bot 
    uint32_t botB = swarm_add_peer("6C:C8:40:8A:89:90");   // second Aegis bot’s MAC
    // uint32_t botC = swarm_add_peer("24:6F:28:AA:BB:02");   // third Aegis bot's MAC
    static HeartbeatArgs hb_args{botA, botB};

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