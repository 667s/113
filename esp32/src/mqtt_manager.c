/**
 * mqtt_manager.c - MQTT 通信管理实现
 *
 * 使用 ESP-IDF 原生 esp_mqtt_client:
 *   - 自动重连
 *   - QoS 1
 *   - 订阅后端开门/拒绝指令主题
 */

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"

#include "config.h"
#include "mqtt_manager.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_callback_t          s_user_callback = NULL;
static int                      s_connected = 0;

// ==================== MQTT 事件处理 ====================
static void mqtt_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        s_connected = 1;
        ESP_LOGI(TAG, "Connected to broker!");
        // 订阅后端指令主题 (dorm/door/cmd)
        esp_mqtt_client_subscribe(s_client, TOPIC_CMD, 1);
        ESP_LOGI(TAG, "Subscribed to: %s", TOPIC_CMD);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = 0;
        ESP_LOGW(TAG, "Disconnected, auto-reconnecting...");
        break;

    case MQTT_EVENT_DATA:
        // 收到消息 → 回调用户函数
        if (s_user_callback) {
            char topic_buf[128] = {0};
            char payload_buf[512] = {0};
            int tlen = ev->topic_len < 127 ? ev->topic_len : 127;
            int plen = ev->data_len < 511 ? ev->data_len : 511;
            memcpy(topic_buf, ev->topic, tlen);
            memcpy(payload_buf, ev->data, plen);
            s_user_callback(topic_buf, payload_buf, plen);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error occurred");
        break;

    default:
        break;
    }
}

// ==================== 初始化 MQTT ====================
esp_err_t mqtt_init(mqtt_callback_t callback)
{
    // 防止重复初始化导致内存泄漏
    if (s_client != NULL) {
        ESP_LOGW(TAG, "Client already exists, destroying old one first...");
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        s_connected = 0;
    }

    s_user_callback = callback;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .session.keepalive = 60,
        .network.disable_auto_reconnect = false,
        .session.message_retransmit_timeout = 10000,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "MQTT client started, broker: %s", MQTT_BROKER_URI);
    return ESP_OK;
}

// ==================== 发布消息 ====================
int mqtt_publish(const char *topic, const char *payload)
{
    if (!s_client || !s_connected) {
        ESP_LOGW(TAG, "Not connected, cannot publish");
        return -1;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Publish failed to topic: %s", topic);
    } else {
        ESP_LOGI(TAG, "Published [%s]: %s", topic, payload);
    }
    return msg_id;
}

int mqtt_is_connected(void)
{
    return s_connected;
}
