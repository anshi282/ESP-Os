idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        display
        wifi_mgr
        audio_svc
        video_svc
        launcher
)
