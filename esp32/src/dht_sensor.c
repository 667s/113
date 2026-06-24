/**
 * dht_sensor.c - DHT11 温湿度传感器 (参考示例代码: 中断临界区保护)
 *
 * 关键改进:
 *   1. portENTER_CRITICAL 全程锁中断, WiFi 无法干扰 bit-banging 时序
 *   2. ets_delay_us() 替代 esp_rom_delay_us()
 *   3. GPIO_MODE_OUTPUT 替代 GPIO_MODE_OUTPUT_OD
 *   4. 参考代码的 bit 读取方式: 等上升沿 → 延时35us → 直接读电平
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "config.h"
#include "dht_sensor.h"

static const char *TAG = "DHT";
static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

/* 等待电平翻转，超时返回 -1 */
static inline int wait_for_level(int target, int timeout_us)
{
    int waited = 0;
    while (gpio_get_level(PIN_DHT11) != target) {
        if (waited >= timeout_us) return -1;
        ets_delay_us(1);
        waited++;
    }
    return waited;
}

esp_err_t dht_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_DHT11),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(PIN_DHT11, 1);

    ESP_LOGI(TAG, "DHT11 initialized (PIN=%d)", PIN_DHT11);
    return ESP_OK;
}

esp_err_t dht_read(dht_data_t *data)
{
    uint8_t bytes[5] = {0};
    memset(data, 0, sizeof(dht_data_t));

    /* === 关键: 锁中断，WiFi 不能打断 DHT11 微秒级单总线时序 === */
    portENTER_CRITICAL(&s_spinlock);

    /* Step 1: MCU 发起始信号 — 拉低 18ms, 拉高 30us */
    gpio_set_direction(PIN_DHT11, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_DHT11, 0);
    ets_delay_us(18000);
    gpio_set_level(PIN_DHT11, 1);
    ets_delay_us(30);

    /* Step 2: DHT11 应答 — 拉低 80us → 拉高 80us */
    gpio_set_direction(PIN_DHT11, GPIO_MODE_INPUT);
    if (wait_for_level(0, 120) < 0) { portEXIT_CRITICAL(&s_spinlock); goto fail; }
    if (wait_for_level(1, 120) < 0) { portEXIT_CRITICAL(&s_spinlock); goto fail; }
    if (wait_for_level(0, 120) < 0) { portEXIT_CRITICAL(&s_spinlock); goto fail; }

    /* Step 3: 读 40 位数据 (参考实现: 等上升沿 → 延 35us → 读电平 → 等下降沿) */
    for (int byte = 0; byte < 5; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            if (wait_for_level(1, 120) < 0) { portEXIT_CRITICAL(&s_spinlock); goto fail; }
            ets_delay_us(35);
            bytes[byte] <<= 1;
            if (gpio_get_level(PIN_DHT11) == 1) bytes[byte] |= 1;
            if (wait_for_level(0, 120) < 0) { portEXIT_CRITICAL(&s_spinlock); goto fail; }
        }
    }

    portEXIT_CRITICAL(&s_spinlock);

    /* Step 4: 校验 */
    if ((bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4]) {
        ESP_LOGW(TAG, "Checksum fail: %02X+%02X+%02X+%02X != %02X",
                 bytes[0], bytes[1], bytes[2], bytes[3], bytes[4]);
        return ESP_FAIL;
    }

    data->temperature = (float)bytes[2];
    data->humidity    = (float)bytes[0];
    data->is_over_temp  = (data->temperature > TEMP_MAX) ? 1 : 0;
    data->is_over_humid = (data->humidity > HUMIDITY_MAX) ? 1 : 0;

    ESP_LOGI(TAG, "Temp: %.1fC | Humidity: %.1f%%",
             data->temperature, data->humidity);
    return ESP_OK;

fail:
    ESP_LOGW(TAG, "DHT read timeout (no response or broken wire)");
    return ESP_FAIL;
}
