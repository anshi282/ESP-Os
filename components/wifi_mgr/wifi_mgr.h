#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialise WiFi manager.
 *        Loads saved credentials from NVS and connects automatically.
 *        Spawns an internal FreeRTOS task; non-blocking.
 */
esp_err_t wifi_mgr_init(void);

/**
 * @brief Connect to a new WiFi network and persist credentials in NVS.
 * @param ssid     Network SSID (max 32 chars)
 * @param password Network password (max 64 chars)
 */
esp_err_t wifi_mgr_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from current WiFi network.
 */
esp_err_t wifi_mgr_disconnect(void);

/**
 * @brief Returns true if station has an IP address.
 */
bool wifi_mgr_is_connected(void);

/**
 * @brief Get current IP address string (or NULL if not connected).
 */
const char *wifi_mgr_get_ip(void);
