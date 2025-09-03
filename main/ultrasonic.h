// ultrasonic.h
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void ultrasonic_init(void);
float hcsr_read_cm(void);
float hcsr_read_cm_med(int samples, int delay_ms);

#ifdef __cplusplus
}
#endif
