#include "launcher.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "music_app.h"
#include "video_app.h"
#include "settings_app.h"

static const char *TAG = "LAUNCHER";
static lv_obj_t   *s_screen = NULL;

// ── Button callbacks ─────────────────────────────────────────
static void btn_music_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Music app");
    music_app_open();
}

static void btn_video_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Video app");
    video_app_open();
}

static void btn_settings_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Settings app");
    settings_app_open();
}

// ── Helper: create one launcher button ───────────────────────
static lv_obj_t *make_app_btn(lv_obj_t *parent,
                               const char *label,
                               lv_color_t bg_color,
                               lv_event_cb_t cb)
{
    // Card style
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 80, 80);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_darken(bg_color, 50), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(lbl);

    return btn;
}

// ── Public ───────────────────────────────────────────────────
void launcher_show(void)
{
    if (lvgl_port_lock(0) == false) return;

    // Root screen
    s_screen = lv_scr_act();
    lv_obj_set_style_bg_color(s_screen,
        lv_color_hex(0x1A1A2E), LV_PART_MAIN);   // dark navy

    // ── Title ────────────────────────────────────────────────
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "myOS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE0E0FF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    // ── Subtitle ─────────────────────────────────────────────
    lv_obj_t *sub = lv_label_create(s_screen);
    lv_label_set_text(sub, "Select an App");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8888AA), LV_PART_MAIN);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 44);

    // ── App grid (flex row) ───────────────────────────────────
    lv_obj_t *grid = lv_obj_create(s_screen);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(70));
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_app_btn(grid, LV_SYMBOL_AUDIO "\nMusic",
                 lv_color_hex(0x6C63FF), btn_music_cb);

    make_app_btn(grid, LV_SYMBOL_VIDEO "\nVideo",
                 lv_color_hex(0xFF6584), btn_video_cb);

    make_app_btn(grid, LV_SYMBOL_SETTINGS "\nSettings",
                 lv_color_hex(0x43B89C), btn_settings_cb);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Launcher displayed");
}

void launcher_return(void)
{
    // Delete current screen widgets and re-show launcher
    if (lvgl_port_lock(0)) {
        lv_obj_clean(lv_scr_act());
        lvgl_port_unlock();
    }
    launcher_show();
}
