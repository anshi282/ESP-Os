idf_component_register(
    SRCS "video_app.c"
    INCLUDE_DIRS "."
    REQUIRES
        lvgl
        esp_lvgl_port
        video_svc
)
