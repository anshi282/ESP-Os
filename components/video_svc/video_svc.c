#include "video_svc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <string.h>

// JPEG decode — esp_jpeg is included in ESP-IDF 5.x
#include "esp_jpeg_dec.h"

static const char *TAG = "VIDEO_SVC";

// ── Config ───────────────────────────────────────────────────
#define JPEG_BUF_SIZE   (20 * 1024)            // 20 KB JPEG input
#define RGB_BUF_SIZE    (320 * 240 * 2)        // 320×240 RGB565

// ── State ─────────────────────────────────────────────────────
static QueueHandle_t s_video_queue  = NULL;
static bool          s_playing      = false;
static uint8_t      *s_jpeg_buf     = NULL;
static uint8_t      *s_rgb_buf      = NULL;

// ── MJPEG boundary search ────────────────────────────────────
// MJPEG stream: --boundary\r\nContent-Type: image/jpeg\r\n\r\n<JPEG data>\r\n
static const char *BOUNDARY_SOI  = "\xff\xd8";   // JPEG start
static const char *BOUNDARY_EOI  = "\xff\xd9";   // JPEG end

// ── Decode one JPEG frame → RGB565 ──────────────────────────
static esp_err_t decode_jpeg(const uint8_t *jpeg, size_t jpeg_len,
                               uint8_t *out_rgb, size_t *out_len)
{
    jpeg_dec_config_t dec_cfg = {
        .output_type = JPEG_RAW_TYPE_RGB565_BE,
        .rotate      = JPEG_ROTATE_0D,
    };
    jpeg_dec_handle_t dec;
    jpeg_dec_open(&dec_cfg, &dec);

    jpeg_dec_io_t io = {
        .inbuf     = (uint8_t *)jpeg,
        .inbuf_len = jpeg_len,
        .outbuf    = out_rgb,
    };
    jpeg_dec_header_info_t info;
    esp_err_t err = jpeg_dec_process(dec, &io, &info);
    *out_len = info.width * info.height * 2;
    jpeg_dec_close(dec);
    return err;
}

// ── HTTP event: accumulate body ──────────────────────────────
typedef struct {
    uint8_t *buf;
    size_t   len;
    size_t   capacity;
    lv_obj_t *canvas;
    bool      stop;
} http_ctx_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_ctx_t *ctx = (http_ctx_t *)evt->user_data;
    if (!ctx) return ESP_OK;

    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        uint8_t *data  = (uint8_t *)evt->data;
        size_t   dlen  = evt->data_len;

        // Append chunk
        if (ctx->len + dlen < ctx->capacity) {
            memcpy(ctx->buf + ctx->len, data, dlen);
            ctx->len += dlen;
        }

        // Search for complete JPEG frame (SOI … EOI)
        for (size_t i = 0; i + 1 < ctx->len; i++) {
            if (ctx->buf[i] == 0xFF && ctx->buf[i+1] == 0xD8) {
                // Found SOI — search for EOI from here
                for (size_t j = i + 2; j + 1 < ctx->len; j++) {
                    if (ctx->buf[j] == 0xFF && ctx->buf[j+1] == 0xD9) {
                        // Complete frame: [i .. j+1]
                        size_t frame_len = j + 2 - i;
                        size_t rgb_len;
                        if (decode_jpeg(ctx->buf + i, frame_len,
                                        s_rgb_buf, &rgb_len) == ESP_OK) {
                            // Push to LVGL canvas
                            if (lvgl_port_lock(0)) {
                                if (ctx->canvas) {
                                    lv_canvas_set_buffer(ctx->canvas,
                                        s_rgb_buf, 320, 240,
                                        LV_COLOR_FORMAT_RGB565);
                                    lv_obj_invalidate(ctx->canvas);
                                }
                                lvgl_port_unlock();
                            }
                        }
                        // Shift remaining data
                        size_t remain = ctx->len - (j + 2);
                        memmove(ctx->buf, ctx->buf + j + 2, remain);
                        ctx->len = remain;
                        break;
                    }
                }
                break;
            }
        }
    }
    return ESP_OK;
}

// ── Video task ────────────────────────────────────────────────
static void video_task(void *arg)
{
    video_msg_t msg;
    ESP_LOGI(TAG, "Video task running on core %d", xPortGetCoreID());

    // Allocate buffers in PSRAM
    s_jpeg_buf = heap_caps_malloc(JPEG_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_rgb_buf  = heap_caps_malloc(RGB_BUF_SIZE,  MALLOC_CAP_SPIRAM);
    if (!s_jpeg_buf || !s_rgb_buf) {
        ESP_LOGE(TAG, "PSRAM alloc failed — PSRAM enabled in menuconfig?");
        vTaskDelete(NULL);
        return;
    }

    esp_http_client_handle_t client = NULL;

    for (;;) {
        if (xQueueReceive(s_video_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (msg.cmd == VIDEO_CMD_STOP) {
                if (client) {
                    esp_http_client_close(client);
                    esp_http_client_cleanup(client);
                    client = NULL;
                }
                s_playing = false;
                ESP_LOGI(TAG, "Stopped");
            }

            if (msg.cmd == VIDEO_CMD_PLAY || msg.cmd == VIDEO_CMD_SET_URL) {
                if (client) {
                    esp_http_client_close(client);
                    esp_http_client_cleanup(client);
                }

                http_ctx_t ctx = {
                    .buf      = s_jpeg_buf,
                    .len      = 0,
                    .capacity = JPEG_BUF_SIZE,
                    .canvas   = msg.canvas,
                    .stop     = false,
                };

                esp_http_client_config_t http_cfg = {
                    .url            = msg.url,
                    .event_handler  = http_event_handler,
                    .user_data      = &ctx,
                    .buffer_size    = 4096,
                    .timeout_ms     = 10000,
                    .keep_alive_enable = true,
                };
                client = esp_http_client_init(&http_cfg);
                esp_err_t err = esp_http_client_perform(client);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "HTTP failed: %s", esp_err_to_name(err));
                }
                s_playing = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ── Public API ────────────────────────────────────────────────
esp_err_t video_svc_init(void)
{
    s_video_queue = xQueueCreate(4, sizeof(video_msg_t));
    if (!s_video_queue) return ESP_ERR_NO_MEM;

    xTaskCreatePinnedToCore(video_task, "video_task",
                             8192, NULL, 4, NULL, 1 /*Core 1*/);
    return ESP_OK;
}

esp_err_t video_svc_play(const char *url, lv_obj_t *canvas)
{
    video_msg_t msg = {.cmd = VIDEO_CMD_PLAY, .canvas = canvas};
    strlcpy(msg.url, url, sizeof(msg.url));
    return (xQueueSend(s_video_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
               ? ESP_OK : ESP_FAIL;
}

esp_err_t video_svc_stop(void)
{
    video_msg_t msg = {.cmd = VIDEO_CMD_STOP};
    return (xQueueSend(s_video_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
               ? ESP_OK : ESP_FAIL;
}

bool video_svc_is_playing(void) { return s_playing; }
