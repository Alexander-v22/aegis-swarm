#include "ultrasonic.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"

//define the need pins for this 
#define US_TRIG_GPIO GPIO_NUM_5
#define US_ECHO_GPIO GPIO_NUM_34
#define US_TRIGGER_PULSE_US 10
#define US_TIMEOUT_US 25000  // need so that the program fails from getting stuck if no echo is detected

#define US_ERR_NO_RISE (-1.0f) // no rising edge seen
#define US_ERR_NO_FALL (-2.0f) // stuck high / out-of-range
#define US_ERR_ALL_SAMPLES_BAD  (-3.0f)// median: no valid readings

// Converts  ECHO high-time singals that the HC-SR04 uses into cm
static const char* TAG = "HC-SR04";

static inline float us_to_cm(uint32_t echo_us){
    // Multiplies the echo signal by the speed of sound to determine distance traveld then divides by two for round trip
    return (echo_us * 0.0343) / 2.0f; 
}

//Congfigure HC-SR04
 void ultrasonic_init(void){
    //trig as output and we set it to start low
    gpio_config_t trig_config ={};
    trig_config.pin_bit_mask = 1ULL << US_TRIG_GPIO;
    trig_config.mode = GPIO_MODE_OUTPUT;
    trig_config.pull_up_en = GPIO_PULLUP_DISABLE;
    trig_config.pull_down_en =  GPIO_PULLDOWN_DISABLE;
    trig_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&trig_config);
    gpio_set_level(US_TRIG_GPIO, 0);

    gpio_config_t echo_config = {};
    echo_config.pin_bit_mask = 1ULL << US_ECHO_GPIO;
    echo_config.mode = GPIO_MODE_INPUT;
    echo_config.pull_up_en = GPIO_PULLUP_DISABLE;
    echo_config.pull_down_en =  GPIO_PULLDOWN_DISABLE;
    echo_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&echo_config);

    ESP_LOGI(TAG, "INITIALIZED");
}

float hcsr_read_cm(void){
    // ensure ECHO is low (avoid starting mid-pulse). Limit guard to about 2 ms. (SAFE GAURD)
    int64_t guard_t0 = esp_timer_get_time();
    while (gpio_get_level(US_ECHO_GPIO) == 1){
        if ((esp_timer_get_time() - guard_t0) > 2000) { // 2 ms guard
            break;
        }
    }

    gpio_set_level(US_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(US_TRIG_GPIO, 1); // This initiates the sensors sound burst
    esp_rom_delay_us(US_TRIGGER_PULSE_US);
    gpio_set_level(US_TRIG_GPIO, 0); // Pulls the trigger pin back to low , concluding the trigger event 

    // Wait for rising edge -> echo pin goes from low to high voltage "rising edge"
    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(US_ECHO_GPIO) == 0 ){
        if ((esp_timer_get_time() - t0) > US_TIMEOUT_US) return US_ERR_NO_RISE; 
    }

    int64_t t_rise = esp_timer_get_time();



    // Wait for falling edge  -> echo pin goes from high to low voltage "falling edge"
    while (gpio_get_level(US_ECHO_GPIO) == 1 ){
        if ((esp_timer_get_time() - t_rise) > US_TIMEOUT_US) return US_ERR_NO_FALL;
        //printf("NO FALL\n");

    }
    int64_t t_fall = esp_timer_get_time();

    // Convert pulse width to centimeters 
    return us_to_cm((uint32_t)(t_fall - t_rise));
}


// for negative numbers and return median (SAFE GUARD)
float hcsr_read_cm_med(int samples, int delay_ms){
    if (samples < 1) samples = 1;
    if (samples > 9) samples = 9;
    if (delay_ms < 70) delay_ms = 70;   // enforce ≥70 ms between pings



    float buffer[9]; // 9 -> to match the max number of allowed samples 
    
    int n = 0;
    for(int i = 0; i < samples; ++i ){
        float d = hcsr_read_cm();
        if(d >= 0.0f) buffer[n++] = d; //ignoring all negative numbers 
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    if (n == 0) return US_ERR_ALL_SAMPLES_BAD;

        // tiny insertion sort for the n valid readings (lowest to highest)
        for (int i = 1; i < n; ++i) {
            float key = buffer[i];
            int j = i - 1;
            while (j >= 0 && buffer[j] > key) { buffer[j+1] = buffer[j]; --j; }
            buffer[j+1] = key;
        }
        return buffer[n/2]; // median
    }