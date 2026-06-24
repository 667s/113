/**
 * wifi_manager.h - WiFi 连接管理 (基于 esp_wifi + esp_netif)
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * 初始化并启动 WiFi Station 模式
 * 调用后会自动连接，重试次数由 WIFI_MAX_RETRY 控制
 */
esp_err_t wifi_init_sta(void);

/**
 * 检查 WiFi 是否已连接
 * @return 1=已连接, 0=未连接
 */
int wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
