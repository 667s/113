/**
 * dht_sensor.h - DHT11 温湿度传感器 (RMT 精确时序)
 */

#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** DHT11 读数结果 */
typedef struct {
    float temperature;   // 温度 (℃)
    float humidity;      // 湿度 (%)
    int   is_over_temp;  // 温度超标
    int   is_over_humid; // 湿度超标
} dht_data_t;

/**
 * 初始化 DHT11 (配置 RMT 通道 + GPIO)
 */
esp_err_t dht_init(void);

/**
 * 读取一次温湿度
 * @param data 输出: 温湿度数据
 * @return ESP_OK 成功, ESP_FAIL 读取失败
 */
esp_err_t dht_read(dht_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // DHT_SENSOR_H
