/**
 * buzzer.h - 蜂鸣器控制模块 (LEDC PWM)
 */

#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** 蜂鸣告警模式 */
typedef enum {
    ALARM_PIR,           // 长响2秒 (人体红外告警)
    ALARM_TEMP,          // 间歇短促响 (温度超标)
    ALARM_HUMIDITY,      // 间歇短促响 (湿度超标)
    ALARM_UNAUTHORIZED   // 短促响3声 (未授权刷卡)
} alarm_pattern_t;

/** 初始化蜂鸣器 (LEDC PWM) */
esp_err_t buzzer_init(void);

/** 按模式触发蜂鸣器 */
void buzzer_alarm(alarm_pattern_t pattern);

/** 立即停止蜂鸣 */
void buzzer_stop(void);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
