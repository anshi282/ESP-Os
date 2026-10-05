#include "display.h"
#include "pins.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7789.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "DISPLAY";

// Handles kept for internal use
static esp_lcd_panel_handle_t   s_panel   = NULL;
static esp_lcd_panel_io_handle_t s_io     = NULL;
static lv_disp_t               *s_disp   = NULL;

// ── LVGL flush callback ──────────────────────────────────────
static bool lvgl_flush_cb(esp_lcd_panel_io_handle_t panel_io,
                           esp_lcd_panel_io_event_data_t *edata,
                           void *user_ctx)
{
    lv_disp_t *disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

// ── Public: init ─────────────────────────────────────────────
esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initialising SPI bus");

    // 1. SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_LCD_MOSI,
        .miso_io_num     = -1,              // not used
        .sclk_io_num     = PIN_LCD_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // 2. LCD panel IO (SPI)
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = PIN_LCD_DC,
        .cs_gpio_num       = PIN_LCD_CS,
        .pclk_hz           = LCD_SPI_CLOCK_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = lvgl_flush_cb,  // called when DMA done
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &s_io));

    // 3. ST7789 panel
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .color_space    = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel));

    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_invert_color(s_panel, true);    // ST7789 needs invert
    esp_lcd_panel_set_gap(s_panel, 0, 0);
    esp_lcd_panel_mirror(s_panel, true, false);   // landscape orientation
    esp_lcd_panel_disp_on_off(s_panel, true);
    ESP_LOGI(TAG, "ST7789 panel up");

    // 4. Backlight ON
    gpio_config_t bl_cfg = {
        .pin_bit_mask = BIT64(PIN_LCD_BL),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(PIN_LCD_BL, 1);

    // 5. LVGL port
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority   = 4,
        .task_stack_size = 8192,
        .task_affinity   = 0,             // Core 0
        .task_max_sleep_ms = 500,
        .timer_period_ms   = 5,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 6. Add display to LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = s_io,
        .panel_handle  = s_panel,
        .buffer_size   = LVGL_BUF_PIXELS,
        .double_buffer = true,
        .hres          = LCD_H_RES,
        .vres          = LCD_V_RES,
        .monochrome    = false,
        .color_format  = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy  = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma    = true,
            .buff_spiram = false,
        },
    };
    s_disp = lvgl_port_add_disp(&disp_cfg);
    if (!s_disp) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return ESP_FAIL;
    }

    // Pass disp handle to flush callback via user_ctx
    esp_lcd_panel_io_register_event_callbacks(
        s_io,
        &(esp_lcd_panel_io_callbacks_t){ .on_color_trans_done = lvgl_flush_cb },
        s_disp);

    ESP_LOGI(TAG, "LVGL ready, display %dx%d", LCD_H_RES, LCD_V_RES);
    return ESP_OK;
}

// ── Public: backlight ────────────────────────────────────────
void display_set_backlight(uint8_t brightness)
{
    // Simple on/off via GPIO (PWM upgrade optional)
    gpio_set_level(PIN_LCD_BL, brightness > 0 ? 1 : 0);
}
