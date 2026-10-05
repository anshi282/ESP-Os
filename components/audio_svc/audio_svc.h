#pragma once

#include "esp_err.h"
#include <stdbool.h>

// ── Commands sent to audio task via queue ────────────────────
typedef enum {
    AUDIO_CMD_PLAY,     // start/resume stream
    AUDIO_CMD_STOP,     // stop & free pipeline
    AUDIO_CMD_SET_URL,  // change URL (also starts playback)
    AUDIO_CMD_SET_VOL,  // set volume 0-100
} audio_cmd_t;

typedef struct {
    audio_cmd_t cmd;
    char        url[256];   // used for SET_URL
    int         volume;     // used for SET_VOL (0-100)
} audio_msg_t;

/**
 * @brief Initialise audio service and spawn FreeRTOS task.
 *        Task runs on Core 1 with priority 5.
 */
esp_err_t audio_svc_init(void);

/**
 * @brief Send a command to the audio service (non-blocking).
 */
esp_err_t audio_svc_send(const audio_msg_t *msg);

/**
 * @brief Convenience: play a URL immediately.
 */
esp_err_t audio_svc_play(const char *url);

/**
 * @brief Convenience: stop current stream.
 */
esp_err_t audio_svc_stop(void);

/**
 * @brief Set volume (0-100).
 */
esp_err_t audio_svc_set_volume(int vol);

/**
 * @brief Returns true if audio is currently playing.
 */
bool audio_svc_is_playing(void);
