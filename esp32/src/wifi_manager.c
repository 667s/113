/**
 * wifi_manager.c - WiFi 连接管理实现
 *
 * 使用 ESP-IDF 原生 WiFi API:
 *   - esp_netif_init / esp_netif_create_default_wifi_sta
 *   - esp_wifi_init / esp_wifi_set_mode / esp_wifi_start
 *   - esp_event_handler 注册 WiFi 事件回调
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "config.h"

static const char *TAG = "WiFi";

// FreeRTOS 事件组: 标记 WiFi 连接成功 / 失败
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_count = 0;
static int s_connected = 0;

// ==================== WiFi 事件处理 ====================
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA started, connecting to AP...");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *ev =
                (wifi_event_sta_disconnected_t *)event_data;
            s_connected = 0;
            if (s_retry_count < WIFI_MAX_RETRY) {
                ESP_LOGW(TAG, "Disconnected (reason=%d), reconnecting (%d/%d)...",
                         ev->reason, s_retry_count + 1, WIFI_MAX_RETRY);
                esp_wifi_connect();
                s_retry_count++;
            } else {
                ESP_LOGE(TAG, "WiFi connect failed after %d retries!", WIFI_MAX_RETRY);
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            break;
        }
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        s_connected = 1;
        s_retry_count = 0;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ==================== 初始化 WiFi STA ====================
esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    // 1. 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 2. 初始化 WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 3. 注册事件回调
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, NULL));

    // 4. 配置 WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_OPEN, // 允许开放网络
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta() done, SSID: %s", WIFI_SSID);

    // 5. 等待连接结果
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "WiFi connection failed!");
        return ESP_FAIL;
    }
}

int wifi_is_connected(void)
{
    return s_connected;
}
