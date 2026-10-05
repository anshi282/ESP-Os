#pragma once

#include "esp_err.h"
#include <stdbool.h>

// ── Commands ─────────────────────────────────────────────────
typedef enum {
    VIDEO_CMD_PLAY,    // start MJPEG stream from URL
    VIDEO_CMD_STOP,    // stop stream
    VIDEO_CMD_SET_URL, // change URL
} video_cmd_t;

typedef struct {
    video_cmd_t cmd;
    char        url[256];
    lv_obj_t   *canvas;   // LVGL canvas to draw frames onto
} video_msg_t;

/**
 * @brief Initialise video service (spawns FreeRTOS task on Core 1).
 */
esp_err_t video_svc_init(void);

/**
 * @brief Play an MJPEG stream onto an LVGL canvas.
 */
esp_err_t video_svc_play(const char *url, lv_obj_t *canvas);

/**
 * @brief Stop the video stream.
 */
esp_err_t video_svc_stop(void);

/**
 * @brief Returns true if video is currently streaming.
 */
bool video_svc_is_playing(void);
