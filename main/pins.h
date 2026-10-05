#pragma once

// ============================================================
//  pins.h  –  ALL GPIO assignments in one place
//  Edit ONLY this file when changing hardware/board.
// ============================================================

// ─── ST7789 SPI Display (320×240) ───────────────────────────
#define PIN_LCD_SCK      18   // SPI Clock
#define PIN_LCD_MOSI     19   // SPI MOSI (data to screen)
#define PIN_LCD_CS       15   // Chip Select
#define PIN_LCD_DC       16   // Data / Command
#define PIN_LCD_RST      17   // Reset
#define PIN_LCD_BL       14   // Backlight (GPIO HIGH = on)

// Screen resolution
#define LCD_H_RES        320
#define LCD_V_RES        240
#define LCD_SPI_CLOCK_HZ (40 * 1000 * 1000)   // 40 MHz

// ─── MAX98357A I2S Amplifier ────────────────────────────────
#define PIN_I2S_BCLK      7   // Bit Clock
#define PIN_I2S_LRC       8   // Left/Right Clock (LRCLK / WS)
#define PIN_I2S_DIN       6   // Data IN to amplifier

// I2S port
#define I2S_PORT_NUM      I2S_NUM_0

// ─── Physical Buttons (optional) ────────────────────────────
#define PIN_BTN_BACK      0   // BOOT button — reused as Back key

// ─── LVGL framebuffer ───────────────────────────────────────
// Draw buffer size: 1/10 of total screen pixels (in 16-bit words)
#define LVGL_BUF_PIXELS   (LCD_H_RES * LCD_V_RES / 10)
