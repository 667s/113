/**
 * servo_control.c - SG90 舵机控制实现
 *
 * SG90 控制信号:
 *   - 频率: 50Hz (周期 20ms)
 *   - 0°:   0.5ms 脉宽 → duty = 0.5/20 * 4096 = 102  (12-bit)
 *   - 90°:  1.5ms 脉宽 → duty = 1.5/20 * 4096 = 307
 *   - 180°: 2.5ms 脉宽 → duty = 2.5/20 * 4096 = 512
 *
 * LEDC 12-bit 分辨率: 0 ~ 4095
 * 实际上 0° 和 90° 需要根据实际舵机微调
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "config.h"
#include "servo_control.h"

static const char *TAG = "Servo";

// 12-bit resolution → 4096 steps
// 50Hz → 20ms period
// 0.5ms / 20ms * 4096 ≈ 102
// 1.5ms / 20ms * 4096 ≈ 307
// 2.5ms / 20ms * 4096 ≈ 512

// 映射角度到 duty (0°→0.5ms, 每度约0.0111ms)
// duty = (0.5 + angle * 0.01111) / 20 * 4096
#define ANGLE_TO_DUTY(deg)  (uint32_t)((0.5f + (deg) * 0.01111f) / 20.0f * 4096.0f)

static int s_is_open = 0;

/** 设置舵机角度 */
static void servo_set_angle(int angle)
{
    uint32_t duty = ANGLE_TO_DUTY(angle);
    ledc_set_duty(SERVO_PWM_MODE, SERVO_PWM_CHANNEL, duty);
    ledc_update_duty(SERVO_PWM_MODE, SERVO_PWM_CHANNEL);
}

esp_err_t servo_init(void)
{
    // 定时器配置 (50Hz PWM)
    ledc_timer_config_t timer_cfg = {
        .speed_mode = SERVO_PWM_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = SERVO_PWM_TIMER,
        .freq_hz = SERVO_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // 通道配置
    ledc_channel_config_t ch_cfg = {
        .gpio_num = PIN_SERVO,
        .speed_mode = SERVO_PWM_MODE,
        .channel = SERVO_PWM_CHANNEL,
        .timer_sel = SERVO_PWM_TIMER,
        .duty = ANGLE_TO_DUTY(SERVO_CLOSE_DEG),
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    s_is_open = 0;
    ESP_LOGI(TAG, "SG90 servo initialized (PIN=%d, 50Hz). Door CLOSED.", PIN_SERVO);
    return ESP_OK;
}

void servo_open_door(void)
{
    if (s_is_open) return;

    ESP_LOGI(TAG, "Door OPENING (angle=%d°)", SERVO_OPEN_DEG);
    servo_set_angle(SERVO_OPEN_DEG);
    s_is_open = 1;
}

void servo_close_door(void)
{
    if (!s_is_open) return;

    ESP_LOGI(TAG, "Door CLOSING (angle=%d°)", SERVO_CLOSE_DEG);
    servo_set_angle(SERVO_CLOSE_DEG);
    s_is_open = 0;
}

int servo_is_open(void)
{
    return s_is_open;
}
