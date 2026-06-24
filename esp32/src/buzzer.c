/**
 * buzzer.c - 有源蜂鸣器控制 (简单 GPIO 高低电平)
 *
 * 有源蜂鸣器: 通电就响，不需要 PWM
 * GPIO 高电平 → 蜂鸣器响
 * GPIO 低电平 → 蜂鸣器停
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "config.h"
#include "buzzer.h"

static const char *TAG = "Buzzer";

// 有源蜂鸣器通常接法: VCC→蜂鸣器正→蜂鸣器负→GPIO
// GPIO拉低=导通(响), GPIO拉高=截止(停)
static void buzzer_on(void)
{
    gpio_set_level(PIN_BUZZER, 0);  // 拉低 → 蜂鸣器有电流 → 响
}

static void buzzer_off(void)
{
    gpio_set_level(PIN_BUZZER, 1);  // 拉高 → 无压差 → 停
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
    // 配置为输出，初始拉高 (蜂鸣器VCC-GPIO接法，高电平=停)
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(PIN_BUZZER, 1);  // 拉高 = 蜂鸣器不响

    ESP_LOGI(TAG, "Active buzzer initialized (PIN=%d)", PIN_BUZZER);
    return ESP_OK;
}

void buzzer_alarm(alarm_pattern_t pattern)
{
    switch (pattern) {
    case ALARM_UNAUTHORIZED:
        ESP_LOGI(TAG, "UNAUTHORIZED CARD - 2 short beeps");
        buzzer_beep(150, 2, 200);
        break;

    case ALARM_PIR:
    case ALARM_TEMP:
    case ALARM_HUMIDITY:
        break;
    }
}

void buzzer_stop(void)
{
    buzzer_off();
}
