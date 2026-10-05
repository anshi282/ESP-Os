#include "music_app.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "audio_svc.h"
#include "launcher.h"

static const char *TAG = "MUSIC_APP";

// ── Default radio station (change to any MP3 stream URL) ─────
#define DEFAULT_RADIO_URL \
    "http://icecast.radiofrance.fr/fip-lofi.mp3"

// ── Button callbacks ──────────────────────────────────────────
static void btn_play_cb(lv_event_t *e)
{
    lv_obj_t *btn  = lv_event_get_target(e);
    lv_obj_t *lbl  = lv_obj_get_child(btn, 0);

    if (audio_svc_is_playing()) {
        audio_svc_stop();
        lv_label_set_text(lbl, LV_SYMBOL_PLAY " Play");
        ESP_LOGI(TAG, "Stopped");
    } else {
        audio_svc_play(DEFAULT_RADIO_URL);
        lv_label_set_text(lbl, LV_SYMBOL_STOP " Stop");
        ESP_LOGI(TAG, "Playing: " DEFAULT_RADIO_URL);
    }
}

static void btn_vol_up_cb(lv_event_t *e)   { audio_svc_set_volume(90); }
static void btn_vol_dn_cb(lv_event_t *e)   { audio_svc_set_volume(40); }

static void btn_back_cb(lv_event_t *e)
{
    audio_svc_stop();
    launcher_return();
}

// ── Public ────────────────────────────────────────────────────
void music_app_open(void)
{
    if (!lvgl_port_lock(0)) return;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D0D1A), LV_PART_MAIN);

    // ── Title ─────────────────────────────────────────────────
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, LV_SYMBOL_AUDIO "  Music Player");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xCCBBFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    // ── Radio station label ───────────────────────────────────
    lv_obj_t *station = lv_label_create(scr);
    lv_label_set_text(station, "FIP Lofi Radio");
    lv_obj_set_style_text_font(station, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(station, lv_color_hex(0x888899), LV_PART_MAIN);
    lv_obj_align(station, LV_ALIGN_TOP_MID, 0, 36);

    // ── Equalizer decoration (static bars) ───────────────────
    static const int bar_h[] = {30, 50, 25, 45, 35, 55, 20, 40};
    for (int i = 0; i < 8; i++) {
        lv_obj_t *bar = lv_obj_create(scr);
        lv_obj_set_size(bar, 10, bar_h[i]);
        lv_obj_set_pos(bar, 60 + i * 26, 90 - bar_h[i]);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x6C63FF), LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    }

    // ── Play/Stop button ─────────────────────────────────────
    lv_obj_t *btn_play = lv_btn_create(scr);
    lv_obj_set_size(btn_play, 130, 44);
    lv_obj_align(btn_play, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0x6C63FF), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_play, 22, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *play_lbl = lv_label_create(btn_play);
    lv_label_set_text(play_lbl, LV_SYMBOL_PLAY " Play");
    lv_obj_set_style_text_color(play_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(play_lbl);

    // ── Volume buttons ────────────────────────────────────────
    lv_obj_t *btn_vup = lv_btn_create(scr);
    lv_obj_set_size(btn_vup, 50, 36);
    lv_obj_align(btn_vup, LV_ALIGN_BOTTOM_LEFT, 20, -50);
    lv_obj_set_style_bg_color(btn_vup, lv_color_hex(0x3A3A5C), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_vup, btn_vol_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *vup_lbl = lv_label_create(btn_vup);
    lv_label_set_text(vup_lbl, LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(vup_lbl);

    lv_obj_t *btn_vdn = lv_btn_create(scr);
    lv_obj_set_size(btn_vdn, 50, 36);
    lv_obj_align(btn_vdn, LV_ALIGN_BOTTOM_RIGHT, -20, -50);
    lv_obj_set_style_bg_color(btn_vdn, lv_color_hex(0x3A3A5C), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_vdn, btn_vol_dn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *vdn_lbl = lv_label_create(btn_vdn);
    lv_label_set_text(vdn_lbl, LV_SYMBOL_VOLUME_MID);
    lv_obj_center(vdn_lbl);

    // ── Back button ───────────────────────────────────────────
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 70, 30);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x333344), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, btn_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(btn_back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_lbl);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Music app opened");
}

void music_app_close(void)
{
    audio_svc_stop();
}
