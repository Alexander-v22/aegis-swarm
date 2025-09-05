#pragma once
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize PIR sensor (GPIO config + optional LED).
 */
esp_err_t pir_init(void);

/**
 * @brief Poll PIR state and apply duration filter.
 *
 * - Starts timer when PIR goes HIGH.
 * - When PIR goes LOW, computes how long it stayed HIGH.
 * - Only returns true if HIGH ≥ PIR_MIN_HIGH_MS (likely human).
 *
 * @return true if a valid motion event was detected, false otherwise.
 */
bool pir_check(void);
