#include "audio_svc.h"
#include "pins.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>

// ─────────────────────────────────────────────────────────────
//  NOTE: This component uses ESP-ADF.
//  Install ESP-ADF alongside ESP-IDF, then set $ADF_PATH and
//  add the following to the root CMakeLists.txt:
//      include($ENV{ADF_PATH}/CMakeLists.txt)
//
//  If ESP-ADF is NOT installed yet, the #ifdef guards below
//  allow the project to compile without audio (stub mode).
// ─────────────────────────────────────────────────────────────
#ifdef CONFIG_IDF_TARGET_ESP32S3  // ensures ESP-IDF is loaded

#if __has_include("audio_pipeline.h")
  #define AUDIO_ADF_AVAILABLE 1
  #include "audio_pipeline.h"
  #include "http_stream.h"
  #include "mp3_decoder.h"
  #include "i2s_stream.h"
  #include "audio_element.h"
  #include "audio_event_iface.h"
  #include "esp_audio.h"
#endif

#endif  // CONFIG_IDF_TARGET_ESP32S3

static const char *TAG = "AUDIO_SVC";

// ── State ────────────────────────────────────────────────────
static QueueHandle_t s_audio_queue = NULL;
static bool          s_playing     = false;
static int           s_volume      = 70;

#ifdef AUDIO_ADF_AVAILABLE
static audio_pipeline_handle_t s_pipeline   = NULL;
static audio_element_handle_t  s_http_stream = NULL;
static audio_element_handle_t  s_mp3_decoder = NULL;
static audio_element_handle_t  s_i2s_writer  = NULL;

// ── Pipeline build ──────────────────────────────────────────
static void pipeline_start(const char *url)
{
    if (s_pipeline) {
        audio_pipeline_stop(s_pipeline);
        audio_pipeline_wait_for_stop(s_pipeline);
        audio_pipeline_terminate(s_pipeline);
        audio_pipeline_unregister(s_pipeline, s_http_stream);
        audio_pipeline_unregister(s_pipeline, s_mp3_decoder);
        audio_pipeline_unregister(s_pipeline, s_i2s_writer);
        audio_pipeline_deinit(s_pipeline);
        audio_element_deinit(s_http_stream);
        audio_element_deinit(s_mp3_decoder);
        audio_element_deinit(s_i2s_writer);
        s_pipeline = NULL;
    }

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    s_pipeline = audio_pipeline_init(&pipeline_cfg);

    // HTTP stream
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.type = AUDIO_STREAM_READER;
    s_http_stream = http_stream_init(&http_cfg);

    // MP3 decoder
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    s_mp3_decoder = mp3_decoder_init(&mp3_cfg);

    // I2S stream writer
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.i2s_config.sample_rate  = 44100;
    i2s_cfg.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_cfg.gpio_cfg.bclk = PIN_I2S_BCLK;
    i2s_cfg.gpio_cfg.ws   = PIN_I2S_LRC;
    i2s_cfg.gpio_cfg.dout = PIN_I2S_DIN;
    i2s_cfg.gpio_cfg.din  = -1;
    s_i2s_writer = i2s_stream_init(&i2s_cfg);

    // Register & link
    audio_pipeline_register(s_pipeline, s_http_stream, "http");
    audio_pipeline_register(s_pipeline, s_mp3_decoder, "mp3");
    audio_pipeline_register(s_pipeline, s_i2s_writer,  "i2s");
    const char *link[] = {"http", "mp3", "i2s"};
    audio_pipeline_link(s_pipeline, link, 3);

    // Set URL and run
    audio_element_set_uri(s_http_stream, url);
    audio_pipeline_run(s_pipeline);
    s_playing = true;
    ESP_LOGI(TAG, "Pipeline started: %s", url);
}

static void pipeline_stop(void)
{
    if (!s_pipeline) return;
    audio_pipeline_stop(s_pipeline);
    audio_pipeline_wait_for_stop(s_pipeline);
    audio_pipeline_terminate(s_pipeline);
    audio_pipeline_deinit(s_pipeline);
    audio_element_deinit(s_http_stream);
    audio_element_deinit(s_mp3_decoder);
    audio_element_deinit(s_i2s_writer);
    s_pipeline    = NULL;
    s_http_stream = NULL;
    s_mp3_decoder = NULL;
    s_i2s_writer  = NULL;
    s_playing = false;
    ESP_LOGI(TAG, "Pipeline stopped");
}
#endif  // AUDIO_ADF_AVAILABLE

// ── Audio task (Core 1) ──────────────────────────────────────
static void audio_task(void *arg)
{
    audio_msg_t msg;
    ESP_LOGI(TAG, "Audio task running on core %d", xPortGetCoreID());

    for (;;) {
        if (xQueueReceive(s_audio_queue, &msg, pdMS_TO_TICKS(200)) == pdTRUE) {
            switch (msg.cmd) {
                case AUDIO_CMD_SET_URL:
                case AUDIO_CMD_PLAY:
#ifdef AUDIO_ADF_AVAILABLE
                    pipeline_start(msg.url);
#else
                    ESP_LOGI(TAG, "[STUB] Would play: %s", msg.url);
                    s_playing = true;
#endif
                    break;

                case AUDIO_CMD_STOP:
#ifdef AUDIO_ADF_AVAILABLE
                    pipeline_stop();
#else
                    ESP_LOGI(TAG, "[STUB] Stop");
                    s_playing = false;
#endif
                    break;

                case AUDIO_CMD_SET_VOL:
                    s_volume = msg.volume;
                    ESP_LOGI(TAG, "Volume set to %d", s_volume);
                    // i2s_alc_volume_set() — optional volume control
                    break;
            }
        }
        // If pipeline is running, periodically check for errors/end-of-stream
#ifdef AUDIO_ADF_AVAILABLE
        if (s_playing && s_pipeline) {
            // Minimal health check — production code would use event iface
        }
#endif
    }
}

// ── Public API ───────────────────────────────────────────────
esp_err_t audio_svc_init(void)
{
    s_audio_queue = xQueueCreate(8, sizeof(audio_msg_t));
    if (!s_audio_queue) return ESP_ERR_NO_MEM;

    // Pin to Core 1, priority 5 — keeps UI (Core 0) unaffected
    xTaskCreatePinnedToCore(audio_task, "audio_task",
                             6144, NULL, 5, NULL, 1 /*Core 1*/);
    return ESP_OK;
}

esp_err_t audio_svc_send(const audio_msg_t *msg)
{
    if (!s_audio_queue) return ESP_ERR_INVALID_STATE;
    return (xQueueSend(s_audio_queue, msg, pdMS_TO_TICKS(100)) == pdTRUE)
               ? ESP_OK : ESP_FAIL;
}

esp_err_t audio_svc_play(const char *url)
{
    audio_msg_t msg = {.cmd = AUDIO_CMD_SET_URL};
    strlcpy(msg.url, url, sizeof(msg.url));
    return audio_svc_send(&msg);
}

esp_err_t audio_svc_stop(void)
{
    audio_msg_t msg = {.cmd = AUDIO_CMD_STOP};
    return audio_svc_send(&msg);
}

esp_err_t audio_svc_set_volume(int vol)
{
    audio_msg_t msg = {.cmd = AUDIO_CMD_SET_VOL, .volume = vol};
    return audio_svc_send(&msg);
}

bool audio_svc_is_playing(void)
{
    return s_playing;
}
