/**
 * servo_control.c - SG90 舵机控制 (与示例参考代码完全一致)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "config.h"
#include "servo_control.h"   

static const char *TAG = "Servo";
static int s_is_open = 0;
static void servo_set_angle(int angle);

esp_err_t servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = PIN_SERVO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,           // 与参考代码一致: 初始无 PWM
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));

    // 初始化时设定关门角度 (0°)，建立 PWM 信号
    // SG90 需要持续 PWM 脉冲才能工作，初始 duty=0 时无信号输出
    servo_set_angle(0);
    s_is_open = 0;

    ESP_LOGI(TAG, "SG90 servo init (PIN=%d, 50Hz, 13-bit)", PIN_SERVO);
    return ESP_OK;
}

static void servo_set_angle(int angle)
{
    // 50Hz 13bit, 0.5ms-2.5ms → duty 205-1024 (0°-180°)
    int duty = 205 + (angle * (1024 - 205) / 180);
    ESP_LOGI(TAG, "Set angle=%d° duty=%d", angle, duty);
    // 必须用两步操作: ledc_fade_func_install 需要额外内存且不是必需的
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void servo_open_door(void)
{
    if (s_is_open) return;
    ESP_LOGI(TAG, "Door OPENING");
    servo_set_angle(90);
    s_is_open = 1;
}

void servo_close_door(void)
{
    if (!s_is_open) return;
    ESP_LOGI(TAG, "Door CLOSING");
    servo_set_angle(0);
    s_is_open = 0;
}

int servo_is_open(void)
{                           
    return s_is_open;
}
