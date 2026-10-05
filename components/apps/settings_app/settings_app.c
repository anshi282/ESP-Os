#include "settings_app.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "wifi_mgr.h"
#include "launcher.h"

static const char *TAG = "SETTINGS";

static lv_obj_t *s_ssid_ta   = NULL;
static lv_obj_t *s_pass_ta   = NULL;
static lv_obj_t *s_status    = NULL;
static lv_obj_t *s_ip_label  = NULL;

// ── Periodic status updater (called from btn or timer) ────────
static void refresh_status(void)
{
    if (!s_status) return;
    if (wifi_mgr_is_connected()) {
        char buf[64];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " Connected  IP: %s",
                 wifi_mgr_get_ip() ? wifi_mgr_get_ip() : "...");
        lv_label_set_text(s_status, buf);
        lv_obj_set_style_text_color(s_status, lv_color_hex(0x44FF88), LV_PART_MAIN);
    } else {
        lv_label_set_text(s_status, "Not connected");
        lv_obj_set_style_text_color(s_status, lv_color_hex(0xFF6666), LV_PART_MAIN);
    }
}

// ── Callbacks ─────────────────────────────────────────────────
static void btn_connect_cb(lv_event_t *e)
{
    const char *ssid = lv_textarea_get_text(s_ssid_ta);
    const char *pass = lv_textarea_get_text(s_pass_ta);

    if (ssid && ssid[0]) {
        ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
        lv_label_set_text(s_status, "Connecting…");
        lv_obj_set_style_text_color(s_status, lv_color_hex(0xFFCC44), LV_PART_MAIN);
        wifi_mgr_connect(ssid, pass);
    }
}

static void btn_disconnect_cb(lv_event_t *e)
{
    wifi_mgr_disconnect();
    lv_label_set_text(s_status, "Disconnected");
    lv_obj_set_style_text_color(s_status, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
}

static void btn_back_cb(lv_event_t *e)
{
    launcher_return();
}

static void status_timer_cb(lv_timer_t *t)
{
    if (lvgl_port_lock(0)) {
        refresh_status();
        lvgl_port_unlock();
    }
}

// ── Public ────────────────────────────────────────────────────
void settings_app_open(void)
{
    if (!lvgl_port_lock(0)) return;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0E1621), LV_PART_MAIN);

    // ── Title ─────────────────────────────────────────────────
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS "  Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x43B89C), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // ── SSID ──────────────────────────────────────────────────
    lv_obj_t *ssid_lbl = lv_label_create(scr);
    lv_label_set_text(ssid_lbl, "WiFi SSID:");
    lv_obj_set_style_text_color(ssid_lbl, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_align(ssid_lbl, LV_ALIGN_TOP_LEFT, 16, 44);

    s_ssid_ta = lv_textarea_create(scr);
    lv_obj_set_size(s_ssid_ta, 200, 32);
    lv_obj_align(s_ssid_ta, LV_ALIGN_TOP_LEFT, 16, 60);
    lv_textarea_set_placeholder_text(s_ssid_ta, "Enter SSID");
    lv_textarea_set_one_line(s_ssid_ta, true);

    // ── Password ──────────────────────────────────────────────
    lv_obj_t *pass_lbl = lv_label_create(scr);
    lv_label_set_text(pass_lbl, "Password:");
    lv_obj_set_style_text_color(pass_lbl, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_align(pass_lbl, LV_ALIGN_TOP_LEFT, 16, 100);

    s_pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(s_pass_ta, 200, 32);
    lv_obj_align(s_pass_ta, LV_ALIGN_TOP_LEFT, 16, 116);
    lv_textarea_set_placeholder_text(s_pass_ta, "Enter password");
    lv_textarea_set_password_mode(s_pass_ta, true);
    lv_textarea_set_one_line(s_pass_ta, true);

    // ── Status ────────────────────────────────────────────────
    s_status = lv_label_create(scr);
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_status, LV_ALIGN_TOP_MID, 0, 155);
    refresh_status();

    // ── Connect button ────────────────────────────────────────
    lv_obj_t *btn_con = lv_btn_create(scr);
    lv_obj_set_size(btn_con, 110, 36);
    lv_obj_align(btn_con, LV_ALIGN_BOTTOM_LEFT, 16, -10);
    lv_obj_set_style_bg_color(btn_con, lv_color_hex(0x43B89C), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_con, 18, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_con, btn_connect_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *c_lbl = lv_label_create(btn_con);
    lv_label_set_text(c_lbl, LV_SYMBOL_WIFI " Connect");
    lv_obj_center(c_lbl);

    // ── Disconnect button ─────────────────────────────────────
    lv_obj_t *btn_dis = lv_btn_create(scr);
    lv_obj_set_size(btn_dis, 80, 36);
    lv_obj_align(btn_dis, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_dis, lv_color_hex(0x553344), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_dis, btn_disconnect_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *d_lbl = lv_label_create(btn_dis);
    lv_label_set_text(d_lbl, "Disconnect");
    lv_obj_set_style_text_font(d_lbl, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(d_lbl);

    // ── Back button ───────────────────────────────────────────
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 70, 36);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x222233), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, btn_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *b_lbl = lv_label_create(btn_back);
    lv_label_set_text(b_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(b_lbl);

    // ── Timer to refresh status every 2s ─────────────────────
    lv_timer_create(status_timer_cb, 2000, NULL);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Settings app opened");
}

void settings_app_close(void) {}
