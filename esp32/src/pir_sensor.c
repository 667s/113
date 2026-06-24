/**
 * pir_sensor.c - HC-SR501 人体红外传感器实现
 *
 * HC-SR501:
 *   - HIGH (3.3V) = 检测到人体移动
 *   - LOW  (0V)  = 无人
 *   - 锁定时间: 约 2-3 秒 (模块本身的延时)
 *   - 软件冷却时间: PIR_COOLDOWN_MS (避免反复触发)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"
#include "pir_sensor.h"

static const char *TAG = "PIR";

static int64_t s_last_trigger_time_us = 0;
static int     s_last_state = 0;

esp_err_t pir_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_PIR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    ESP_LOGI(TAG, "HC-SR501 initialized (PIN=%d)", PIN_PIR);
    return ESP_OK;
}

int pir_check(void)
{
    int level = gpio_get_level(PIN_PIR);
    int64_t now = esp_timer_get_time();

    if (level == 1 && (now - s_last_trigger_time_us) > (PIR_INTERVAL_MS * 1000)) {
        s_last_trigger_time_us = now;

        if (!s_last_state) {
            s_last_state = 1;
            ESP_LOGI(TAG, "Motion detected! Someone near the door!");
        }
        return 1;
    }

    if (level == 0 && s_last_state) {
        s_last_state = 0;
        ESP_LOGI(TAG, "Motion cleared.");
    }

    return 0;
}
