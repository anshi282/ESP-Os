idf_component_register(
    SRCS "audio_svc.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/main"
    REQUIRES
        driver
        freertos
        esp_log
)
# Note: When ESP-ADF is enabled, add esp_audio, http_stream,
# mp3_decoder, i2s_stream to REQUIRES above and run:
#   include($ENV{ADF_PATH}/CMakeLists.txt)  in root CMakeLists.txt
