/**
 * pir_sensor.h - HC-SR501 人体红外传感器
 */

#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** 初始化 PIR 传感器 (GPIO 输入) */
esp_err_t pir_init(void);

/**
 * 检查是否检测到人体
 * 内置冷却时间 (PIR_COOLDOWN_MS), 避免持续触发
 * @return 1=检测到人体, 0=无人
 */
int pir_check(void);

#ifdef __cplusplus
}
#endif

#endif // PIR_SENSOR_H
