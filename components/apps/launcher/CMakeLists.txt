idf_component_register(
    SRCS "launcher.c"
    INCLUDE_DIRS "."
    REQUIRES
        lvgl
        esp_lvgl_port
        music_app
        video_app
        settings_app
)
