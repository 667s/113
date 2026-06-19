/**
 * buzzer.c - 蜂鸣器控制实现
 *
 * 使用 LEDC (LED PWM Controller) 驱动无源蜂鸣器。
 * 多个告警源共用同一个蜂鸣器:
 *   - PIR 人体检测: 长响 2 秒
 *   - 未授权刷卡:   短促响 3 声
 *   - 温度超标:     间歇响
 *   - 湿度超标:     间歇响
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "config.h"
#include "buzzer.h"

static const char *TAG = "Buzzer";

static void buzzer_on(void)
{
    ledc_set_duty(SERVO_PWM_MODE, BUZZER_PWM_CHANNEL, 4095); // 50% duty
    ledc_update_duty(SERVO_PWM_MODE, BUZZER_PWM_CHANNEL);
}

static void buzzer_off(void)
{
    ledc_set_duty(SERVO_PWM_MODE, BUZZER_PWM_CHANNEL, 0);
    ledc_update_duty(SERVO_PWM_MODE, BUZZER_PWM_CHANNEL);
}

/** 通用蜂鸣: 响 duration_ms，重复 times 次，间隔 interval_ms */
static void buzzer_beep(int duration_ms, int times, int interval_ms)
{
    for (int i = 0; i < times; i++) {
        buzzer_on();
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        buzzer_off();
        if (i < times - 1) {
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
        }
    }
}

esp_err_t buzzer_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = SERVO_PWM_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,  // 12-bit → 0~4095
        .timer_num = BUZZER_PWM_TIMER,
        .freq_hz = BUZZER_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num = PIN_BUZZER,
        .speed_mode = SERVO_PWM_MODE,
        .channel = BUZZER_PWM_CHANNEL,
        .timer_sel = BUZZER_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    ESP_LOGI(TAG, "Buzzer initialized (PIN=%d, freq=%dHz)", PIN_BUZZER, BUZZER_PWM_FREQ);
    return ESP_OK;
}

void buzzer_alarm(alarm_pattern_t pattern)
{
    switch (pattern) {
    case ALARM_PIR:
        ESP_LOGI(TAG, "PIR ALARM - Continuous 2s");
        buzzer_on();
        vTaskDelay(pdMS_TO_TICKS(2000));
        buzzer_off();
        break;

    case ALARM_UNAUTHORIZED:
        ESP_LOGI(TAG, "UNAUTHORIZED CARD - 3 short beeps");
        buzzer_beep(150, 3, 200);
        break;

    case ALARM_TEMP:
        ESP_LOGI(TAG, "TEMP ALARM - Intermittent");
        buzzer_beep(300, 3, 300);
        break;

    case ALARM_HUMIDITY:
        ESP_LOGI(TAG, "HUMIDITY ALARM - Intermittent");
        buzzer_beep(300, 3, 300);
        break;
    }
}

void buzzer_stop(void)
{
    buzzer_off();
}
