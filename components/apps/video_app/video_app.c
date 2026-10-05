#include "video_app.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "video_svc.h"
#include "launcher.h"
#include <string.h>

static const char *TAG = "VIDEO_APP";

// Canvas for video frames
static lv_obj_t *s_canvas = NULL;

// ── Text area to type server IP ───────────────────────────────
static lv_obj_t *s_url_ta  = NULL;
static lv_obj_t *s_status  = NULL;

static void update_status(const char *msg, lv_color_t color)
{
    if (!lvgl_port_lock(0)) return;
    if (s_status) {
        lv_label_set_text(s_status, msg);
        lv_obj_set_style_text_color(s_status, color, LV_PART_MAIN);
    }
    lvgl_port_unlock();
}

static void btn_play_cb(lv_event_t *e)
{
    const char *ip = lv_textarea_get_text(s_url_ta);
    char url[128];
    snprintf(url, sizeof(url), "http://%s:8080", ip);
    ESP_LOGI(TAG, "Connecting to: %s", url);
    update_status("Connecting…", lv_color_hex(0xFFCC44));
    video_svc_play(url, s_canvas);
}

static void btn_stop_cb(lv_event_t *e)
{
    video_svc_stop();
    update_status("Stopped", lv_color_hex(0xFF6666));
}

static void btn_back_cb(lv_event_t *e)
{
    video_svc_stop();
    launcher_return();
}

// ── Public ────────────────────────────────────────────────────
void video_app_open(void)
{
    if (!lvgl_port_lock(0)) return;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0A1A), LV_PART_MAIN);

    // ── Title ─────────────────────────────────────────────────
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, LV_SYMBOL_VIDEO "  Video Stream");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF8FAB), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // ── Canvas (video frame destination) ─────────────────────
    s_canvas = lv_canvas_create(scr);
    lv_obj_set_size(s_canvas, 240, 135);   // 16:9 preview at top
    lv_obj_align(s_canvas, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_border_color(s_canvas, lv_color_hex(0xFF6584), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_canvas, 2, LV_PART_MAIN);

    // ── IP input ──────────────────────────────────────────────
    lv_obj_t *ip_label = lv_label_create(scr);
    lv_label_set_text(ip_label, "Server IP:");
    lv_obj_set_style_text_color(ip_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(ip_label, LV_ALIGN_BOTTOM_LEFT, 10, -78);

    s_url_ta = lv_textarea_create(scr);
    lv_obj_set_size(s_url_ta, 140, 32);
    lv_obj_align(s_url_ta, LV_ALIGN_BOTTOM_LEFT, 10, -44);
    lv_textarea_set_placeholder_text(s_url_ta, "192.168.1.xx");
    lv_textarea_set_one_line(s_url_ta, true);

    // ── Status label ─────────────────────────────────────────
    s_status = lv_label_create(scr);
    lv_label_set_text(s_status, "Ready");
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x44FF88), LV_PART_MAIN);
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -8);

    // ── Buttons ───────────────────────────────────────────────
    // Play
    lv_obj_t *btn_play = lv_btn_create(scr);
    lv_obj_set_size(btn_play, 50, 32);
    lv_obj_align(btn_play, LV_ALIGN_BOTTOM_RIGHT, -60, -44);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0xFF6584), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *p_lbl = lv_label_create(btn_play);
    lv_label_set_text(p_lbl, LV_SYMBOL_PLAY);
    lv_obj_center(p_lbl);

    // Stop
    lv_obj_t *btn_stop = lv_btn_create(scr);
    lv_obj_set_size(btn_stop, 50, 32);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT, -5, -44);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0x444455), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_stop, btn_stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *s_lbl = lv_label_create(btn_stop);
    lv_label_set_text(s_lbl, LV_SYMBOL_STOP);
    lv_obj_center(s_lbl);

    // Back
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 70, 28);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2A2A40), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, btn_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *b_lbl = lv_label_create(btn_back);
    lv_label_set_text(b_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(b_lbl);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Video app opened");
}

void video_app_close(void)
{
    video_svc_stop();
    s_canvas = NULL;
}
