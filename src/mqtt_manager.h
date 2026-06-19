/**
 * mqtt_manager.h - MQTT 通信管理 (基于 esp_mqtt_client)
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * MQTT 消息回调函数类型
 * @param topic   消息主题
 * @param payload 消息内容 (字符串)
 * @param len     内容长度
 */
typedef void (*mqtt_callback_t)(const char *topic, const char *payload, int len);

/**
 * 初始化并启动 MQTT 客户端
 * @param callback 收到消息时的回调 (可为 NULL)
 * @return ESP_OK 成功
 */
esp_err_t mqtt_init(mqtt_callback_t callback);

/**
 * 发布消息到指定主题 (QoS 1)
 * @param topic   目标主题
 * @param payload JSON 字符串
 * @return 消息ID (>0 成功), -1 失败
 */
int mqtt_publish(const char *topic, const char *payload);

/**
 * 检查 MQTT 是否已连接
 * @return 1=已连接, 0=未连接
 */
int mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_MANAGER_H
