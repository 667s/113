/**
 * servo_control.h - SG90 舵机控制 (LEDC PWM, 50Hz)
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** 初始化舵机 (LEDC PWM, 50Hz) */
esp_err_t servo_init(void);

/** 开门 (默认 90°) */
void servo_open_door(void);

/** 关门 (默认 0°) */
void servo_close_door(void);

/** 是否开门状态 */
int servo_is_open(void);

#ifdef __cplusplus
}
#endif

#endif // SERVO_CONTROL_H
