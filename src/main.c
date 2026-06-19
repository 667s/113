/**
 * main.c - 校园宿舍智能安防门 主程序 (ESP-IDF 原生)
 *
 * 功能:
 *   1. MFRC522 RFID刷卡 → 授权开锁(SG90) + 上报信息
 *   2. HC-SR501 PIR人体检测 → 蜂鸣器 + MQTT告警
 *   3. DHT11 温湿度采集 → 超标蜂鸣器 + MQTT告警
 *   4. 订阅远程开门指令 (dorm/door/servo/cmd)
 *
 * MQTT 主题:
 *   发布 dorm/door/rfid       - RFID刷卡事件
 *   发布 dorm/door/pir        - 人体红外检测
 *   发布 dorm/door/dht        - 温湿度数据
 *   发布 dorm/door/alarm      - 告警事件
 *   订阅 dorm/door/servo/cmd  - 远程开门指令
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "config.h"
#include "authorized_cards.h"
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

    // 处理远程开门指令
    if (strcmp(topic, TOPIC_SERVO_CMD) == 0) {
        cJSON *root = cJSON_ParseWithLength(payload, len);
        if (root) {
            cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
            if (cmd && cJSON_IsString(cmd) && strcmp(cmd->valuestring, "open") == 0) {
                ESP_LOGI(TAG, "Remote open door command received!");
                servo_open_door();
                vTaskDelay(pdMS_TO_TICKS(2000));
                servo_close_door();
            }
            cJSON_Delete(root);
        }
    }
}

// ==================== 业务处理 ====================

/** RFID 刷卡处理 */
static void handle_rfid(const rfid_result_t *result)
{
    cJSON *root = json_create_base();
    cJSON_AddStringToObject(root, "uid", result->uid);
    cJSON_AddBoolToObject(root, "authorized", result->is_authorized);
    if (result->is_authorized) {
        cJSON_AddStringToObject(root, "holder_name", result->holder_name);
    }

    if (result->is_authorized) {
        // === 授权卡: 开门 2 秒后关闭 ===
        ESP_LOGI(TAG, "✅ Authorized card! Opening door...");
        servo_open_door();
        vTaskDelay(pdMS_TO_TICKS(2000));
        servo_close_door();

        // 发布刷卡事件
        json_publish(TOPIC_RFID, root);
    } else {
        // === 未授权卡: 蜂鸣器告警 ===
        ESP_LOGI(TAG, "❌ Unauthorized card! Alarm triggered.");
        buzzer_alarm(ALARM_UNAUTHORIZED);

        // 发布刷卡事件
        json_publish(TOPIC_RFID, root);

        // 发布告警事件
        cJSON *alarm = json_create_base();
        cJSON_AddStringToObject(alarm, "type", "UNAUTHORIZED");
        char msg[128];
        snprintf(msg, sizeof(msg), "未授权卡片尝试刷卡: %s", result->uid);
        cJSON_AddStringToObject(alarm, "message", msg);
        json_publish(TOPIC_ALARM, alarm);
    }
}

/** PIR 人体检测处理 */
static void handle_pir(int detected)
{
    cJSON *root = json_create_base();
    cJSON_AddBoolToObject(root, "detected", true);

    // 触发蜂鸣器
    buzzer_alarm(ALARM_PIR);

    // 发布 PIR 事件
    json_publish(TOPIC_PIR, root);

    // 发布告警事件 → 后端推送到 App 弹窗
    cJSON *alarm = json_create_base();
    cJSON_AddStringToObject(alarm, "type", "PIR");
    cJSON_AddStringToObject(alarm, "message", "疑似有人非法闯入");
    json_publish(TOPIC_ALARM, alarm);
}

/** DHT11 温湿度处理 */
static void handle_dht(const dht_data_t *data)
{
    // 发布温湿度数据
    cJSON *root = json_create_base();
    cJSON_AddNumberToObject(root, "temperature", data->temperature);
    cJSON_AddNumberToObject(root, "humidity", data->humidity);
    json_publish(TOPIC_DHT, root);

    // 温度超标告警
    if (data->is_over_temp) {
        buzzer_alarm(ALARM_TEMP);

        cJSON *alarm = json_create_base();
        cJSON_AddStringToObject(alarm, "type", "TEMP");
        char msg[64];
        snprintf(msg, sizeof(msg), "温度超标: %.1f°C", data->temperature);
        cJSON_AddStringToObject(alarm, "message", msg);
        cJSON_AddNumberToObject(alarm, "temperature", data->temperature);
        json_publish(TOPIC_ALARM, alarm);
    }

    // 湿度超标告警
    if (data->is_over_humid) {
        buzzer_alarm(ALARM_HUMIDITY);

        cJSON *alarm = json_create_base();
        cJSON_AddStringToObject(alarm, "type", "HUMIDITY");
        char msg[64];
        snprintf(msg, sizeof(msg), "湿度超标: %.1f%%", data->humidity);
        cJSON_AddStringToObject(alarm, "message", msg);
        cJSON_AddNumberToObject(alarm, "humidity", data->humidity);
        json_publish(TOPIC_ALARM, alarm);
    }
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

        // ---- PIR 人体红外检测 ----
        if (pir_check()) {
            handle_pir(1);
        }

        // ---- DHT11 温湿度采集 (定时 30 秒) ----
        if ((now_us - s_last_dht_read_us) > (DHT_INTERVAL_MS * 1000)) {
            s_last_dht_read_us = now_us;
            dht_data_t dht_data;
            if (dht_read(&dht_data) == ESP_OK) {
                handle_dht(&dht_data);
            }
        }

        // ---- MQTT 重连 (10秒间隔，避免内存泄漏) ----
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
