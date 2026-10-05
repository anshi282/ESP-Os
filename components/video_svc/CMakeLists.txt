idf_component_register(
    SRCS "video_svc.c"
    INCLUDE_DIRS "."
    REQUIRES
        freertos
        esp_http_client
        esp_jpeg
        esp_lvgl_port
        lvgl
        esp_heap_caps
)
