/**
 * dht_sensor.c - DHT11 温湿度传感器 (GPIO bit-banging)
 *
 * DHT11 单总线协议:
 *   1. MCU拉低 >18ms → 拉高 20-40us
 *   2. DHT11响应: 80us低 + 80us高
 *   3. 40bit数据: 湿度整数+湿度小数+温度整数+温度小数+校验和
 *   4. bit 0: 50us低 + 26-28us高, bit 1: 50us低 + 70us高
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "config.h"
#include "dht_sensor.h"

static const char *TAG = "DHT";

esp_err_t dht_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_DHT11),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(PIN_DHT11, 1);

    ESP_LOGI(TAG, "DHT11 initialized (PIN=%d)", PIN_DHT11);
    return ESP_OK;
}

esp_err_t dht_read(dht_data_t *data)
{
    memset(data, 0, sizeof(dht_data_t));

    // Step 1: Send start signal (low 20ms, then release)
    gpio_set_direction(PIN_DHT11, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(PIN_DHT11, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_DHT11, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(PIN_DHT11, GPIO_MODE_INPUT);

    // Step 2: Wait for DHT11 response (low ~80us then high ~80us)
    int timeout = 0;
    while (gpio_get_level(PIN_DHT11) == 1) {
        if (++timeout > 200) return ESP_FAIL;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(PIN_DHT11) == 0) {
        if (++timeout > 200) return ESP_FAIL;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(PIN_DHT11) == 1) {
        if (++timeout > 200) return ESP_FAIL;
        esp_rom_delay_us(1);
    }

    // Step 3: Read 40 bits (5 bytes)
    uint8_t bytes[5] = {0};
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            timeout = 0;
            while (gpio_get_level(PIN_DHT11) == 0) {
                if (++timeout > 150) return ESP_FAIL;
                esp_rom_delay_us(1);
            }
            timeout = 0;
            while (gpio_get_level(PIN_DHT11) == 1) {
                if (++timeout > 150) return ESP_FAIL;
                esp_rom_delay_us(1);
            }
            if (timeout > 40) {
                bytes[i] |= (1 << j);
            }
        }
    }

    // Step 4: Checksum
    uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
    if (checksum != bytes[4]) {
        ESP_LOGW(TAG, "Checksum failed: calc=%02X, recv=%02X", checksum, bytes[4]);
        gpio_set_direction(PIN_DHT11, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(PIN_DHT11, 1);
        return ESP_FAIL;
    }

    // Step 5: Parse
    data->humidity    = (float)bytes[0] + (float)bytes[1] * 0.1f;
    data->temperature = (float)bytes[2] + (float)bytes[3] * 0.1f;
    data->is_over_temp  = (data->temperature > TEMP_MAX) ? 1 : 0;
    data->is_over_humid = (data->humidity > HUMIDITY_MAX) ? 1 : 0;

    gpio_set_direction(PIN_DHT11, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(PIN_DHT11, 1);

    ESP_LOGI(TAG, "Temp: %.1fC | Humidity: %.1f%% %s%s",
             data->temperature, data->humidity,
             data->is_over_temp  ? "[TEMP HIGH!]" : "",
             data->is_over_humid ? "[HUMID HIGH!]" : "");

    return ESP_OK;
}
