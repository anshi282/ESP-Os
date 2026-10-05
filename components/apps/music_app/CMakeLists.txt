idf_component_register(
    SRCS "music_app.c"
    INCLUDE_DIRS "."
    REQUIRES
        lvgl
        esp_lvgl_port
        audio_svc
)
