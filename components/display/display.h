#pragma once

#include "esp_err.h"

/**
 * @brief Initialise ST7789 SPI display and LVGL.
 *        Must be called once from app_main before any LVGL calls.
 */
esp_err_t display_init(void);

/**
 * @brief Set backlight brightness (0 = off, 255 = full).
 */
void display_set_backlight(uint8_t brightness);
