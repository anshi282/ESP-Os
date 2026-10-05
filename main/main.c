#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "display.h"
#include "wifi_mgr.h"
#include "audio_svc.h"
#include "video_svc.h"
#include "launcher.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== myOS booting ===");

    // ── 1. Non-Volatile Storage (credentials, settings) ──────
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition wiped, reinitialising");
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS ready");

    // ── 2. Display + LVGL ────────────────────────────────────
    display_init();
    ESP_LOGI(TAG, "Display ready");

    // ── 3. WiFi manager (non-blocking, task inside) ──────────
    wifi_mgr_init();
    ESP_LOGI(TAG, "WiFi manager started");

    // ── 4. Audio service (idle until commanded) ──────────────
    audio_svc_init();
    ESP_LOGI(TAG, "Audio service started");

    // ── 5. Video service (idle until commanded) ──────────────
    video_svc_init();
    ESP_LOGI(TAG, "Video service started");

    // ── 6. Show launcher (home screen) ───────────────────────
    launcher_show();
    ESP_LOGI(TAG, "Launcher shown — boot complete");

    // app_main can return; FreeRTOS tasks keep everything alive
}
