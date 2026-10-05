idf_component_register(
    SRCS "display.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/main"  # for pins.h
    REQUIRES
        driver
        esp_lcd
        esp_lcd_st7789
        esp_lvgl_port
        lvgl
)
