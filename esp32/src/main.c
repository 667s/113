/**
 * main.c - 校园宿舍智能安防门 主程序 (ESP-IDF 原生)
 *
 * 功能:
 *   1. MFRC522刷卡 → MQTT发UID给后端校验 → 后端下发指令 → 开锁/报警
 *   2. PIR人体检测 → MQTT上报(仅前端展示)
 *   3. DHT11温湿度 → MQTT上报(仅前端展示)
 *   4. 订阅 dorm/door/cmd 接收后端开门/拒绝指令
 *
 * 整体流程 (ESP32 ←→ 后端):
 *   ESP32刷卡 → MQTT dorm/door/rfid {"uid":"..."}
 *     → 后端查数据库授权卡表
 *     → MQTT dorm/door/cmd {"cmd":"open"/"deny","uid":"...","authorized":true/false}
 *     → ESP32收到 → 控制舵机/蜂鸣器
 *
 * MQTT 主题:
 *   发布 dorm/door/rfid       - RFID刷卡事件(只发UID, 不判授权)
 *   发布 dorm/door/pir        - 人体红外检测
 *   发布 dorm/door/dht        - 温湿度数据
 *   订阅 dorm/door/cmd        - 后端下发开门/拒绝指令
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "cJSON.h"

#include "config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "rfid_reader.h"
#include "dht_sensor.h"
#include "pir_sensor.h"
#include "buzzer.h"
#include "servo_control.h"

static const char *TAG = "MAIN";

// 定时器
static int64_t s_last_dht_read_us = 0;
static int64_t s_last_pir_report_us = 0;
static int64_t s_last_mqtt_retry_us = 0;

// ==================== JSON 构建辅助函数 ====================

/** 构建基础 JSON 对象 (含时间戳) */
static cJSON *json_create_base(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "timestamp",
                            (double)esp_timer_get_time() / 1000.0);
    return root;
}

/** JSON → 字符串 → 发布 */
static void json_publish(const char *topic, cJSON *root)
{
    char *str = cJSON_PrintUnformatted(root);
    if (str) {
        mqtt_publish(topic, str);
        free(str);
    }
    cJSON_Delete(root);
}

// ==================== MQTT 消息回调 ====================

static void on_mqtt_message(const char *topic, const char *payload, int len)
{
    ESP_LOGI(TAG, "MQTT RX [%s]: %.*s", topic, len, payload);

    // 处理后端下发的开门/拒绝指令
    if (strcmp(topic, TOPIC_CMD) == 0) {
        cJSON *root = cJSON_ParseWithLength(payload, len);
        if (!root) {
            ESP_LOGW(TAG, "Failed to parse cmd JSON");
            return;
        }

        cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
        if (!cmd || !cJSON_IsString(cmd)) {
            ESP_LOGW(TAG, "No 'cmd' field in message");
            cJSON_Delete(root);
            return;
        }

        if (strcmp(cmd->valuestring, "open") == 0) {
            cJSON *uid = cJSON_GetObjectItem(root, "uid");
            ESP_LOGI(TAG, "✅ 开门指令! UID=%s", uid ? uid->valuestring : "?");
            servo_open_door();
            vTaskDelay(pdMS_TO_TICKS(2000));
            servo_close_door();

        } else if (strcmp(cmd->valuestring, "deny") == 0) {
            cJSON *uid = cJSON_GetObjectItem(root, "uid");
            ESP_LOGW(TAG, "❌ 拒绝指令! UID=%s", uid ? uid->valuestring : "?");
            buzzer_alarm(ALARM_UNAUTHORIZED);
        }

        cJSON_Delete(root);
    }
}

// ==================== 业务处理 ====================

/** RFID 刷卡处理 — MQTT 发 UID 给后端校验 (不再用 HTTP) */
static void handle_rfid(const rfid_result_t *result)
{
    ESP_LOGI(TAG, "Card UID: %s → sending to backend via MQTT", result->uid);

    // 发布刷卡事件给后端 (仅发 UID，由后端查数据库判断授权)
    cJSON *root = json_create_base();
    cJSON_AddStringToObject(root, "uid", result->uid);
    json_publish(TOPIC_RFID, root);
    // 后端校验后会通过 dorm/door/cmd 下发 open/deny 指令
}

/** PIR 人体检测处理 (每2秒上报一次状态) */
static void handle_pir(int detected)
{
    cJSON *root = json_create_base();
    cJSON_AddBoolToObject(root, "detected", detected ? true : false);
    json_publish(TOPIC_PIR, root);
}

/** DHT11 温湿度处理 (仅上报数据到前端展示，不触发告警) */
static void handle_dht(const dht_data_t *data)
{
    cJSON *root = json_create_base();
    cJSON_AddNumberToObject(root, "temperature", data->temperature);
    cJSON_AddNumberToObject(root, "humidity", data->humidity);
    json_publish(TOPIC_DHT, root);
}

// ==================== 应用入口 ====================

void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  校园宿舍智能安防门 - ESP32 启动中...");
    ESP_LOGI(TAG, "  ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "===========================================");

    // ---- 1. 初始化 NVS (WiFi 需要) ----
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ---- 2. 连接 WiFi ----
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi init returned error, continuing anyway...");
    }

    // ---- 3. 初始化所有外设 ----
    ESP_LOGI(TAG, "Initializing peripherals...");
    ESP_ERROR_CHECK(rc522_init());
    ESP_ERROR_CHECK(dht_init());
    ESP_ERROR_CHECK(pir_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(servo_init());

    // ---- 4. 连接 MQTT ----
    if (wifi_is_connected()) {
        ESP_LOGI(TAG, "Starting MQTT client...");
        mqtt_init(on_mqtt_message);
        esp_wifi_set_ps(WIFI_PS_NONE);
    } else {
        ESP_LOGW(TAG, "WiFi not connected, MQTT will retry later.");
    }

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  System ready. Monitoring...");
    ESP_LOGI(TAG, "===========================================");

    // ==================== 主循环 ====================
    while (1) {
        int64_t now_us = esp_timer_get_time();

        // ---- RFID 刷卡检测 (高频轮询) ----
        rfid_result_t rfid_result;
        if (rc522_read_card(&rfid_result) == ESP_OK && rfid_result.has_card) {
            handle_rfid(&rfid_result);
        }

        // ---- PIR 人体红外状态上报 (每2秒) ----
        if ((now_us - s_last_pir_report_us) > (PIR_INTERVAL_MS * 1000)) {
            s_last_pir_report_us = now_us;
            int pir_state = gpio_get_level(PIN_PIR);
            handle_pir(pir_state);
        }

        // ---- DHT11 温湿度采集 (每2秒) ----
        if ((now_us - s_last_dht_read_us) > (DHT_INTERVAL_MS * 1000)) {
            s_last_dht_read_us = now_us;
            dht_data_t dht_data;
            if (dht_read(&dht_data) == ESP_OK) {
                handle_dht(&dht_data);
            }
        }

        // ---- MQTT 重连 (10秒间隔) ----
        if (wifi_is_connected() && !mqtt_is_connected() &&
            (now_us - s_last_mqtt_retry_us) > (10000 * 1000)) {
            s_last_mqtt_retry_us = now_us;
            ESP_LOGI(TAG, "MQTT disconnected, reconnecting...");
            mqtt_init(on_mqtt_message);
        }

        // 50ms 循环周期
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
