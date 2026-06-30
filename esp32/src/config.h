/**
 * config.h - 全局配置
 *
 * 修改说明:
 *   1. 填写你的 WiFi SSID 和密码
 *   2. 本机 EMQX Broker IP 已配置
 *   3. 引脚和阈值可按需调整
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// ==================== WiFi 配置 ====================
#define WIFI_SSID           "sxc113"
#define WIFI_PASSWORD       "12345678"
#define WIFI_MAX_RETRY      5           // 最大重试次数

// ==================== MQTT 配置 ====================
// 使用 mqtts:// 协议 (TLS 加密，端口 8883)
#define MQTT_BROKER_URI     "mqtts://10.219.46.147:8883"
#define MQTT_CLIENT_ID      "esp32_dorm_door"

// ==================== AES 加密密钥 (与后端保持一致) ====================
// 16 字节 = 128 bits, hex 编码
#define AES_KEY_HEX         "00112233445566778899AABBCCDDEEFF"

// ==================== TLS CA 证书 (EMQX 自签 CA) ====================
#define MQTT_CA_CERT_PEM \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIDqzCCApOgAwIBAgIUXk0X6Mc8BkfE8y1q/+GyX2KacbcwDQYJKoZIhvcNAQEL\n" \
    "BQAwZTELMAkGA1UEBhMCQ04xEjAQBgNVBAgMCUd1YW5nZG9uZzERMA8GA1UEBwwI\n" \
    "U2hlbnpoZW4xFTATBgNVBAoMDERvcm1TZWN1cml0eTEYMBYGA1UEAwwPRG9ybVNl\n" \
    "Y3VyaXR5LUNBMB4XDTI2MDYzMDA5MjQwMVoXDTM2MDYyNzA5MjQwMVowZTELMAkG\n" \
    "A1UEBhMCQ04xEjAQBgNVBAgMCUd1YW5nZG9uZzERMA8GA1UEBwwIU2hlbnpoZW4x\n" \
    "FTATBgNVBAoMDERvcm1TZWN1cml0eTEYMBYGA1UEAwwPRG9ybVNlY3VyaXR5LUNB\n" \
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAos9a4Iu25Dh9T3ytpObE\n" \
    "yyL7mXVlyF2N3cVzudWwj3uSJZUl7o/qNig2rVJWte97GCUdwLiwtQ5y3djlNUdz\n" \
    "rQ8wYdQsO6Gi9I3DcEqywp95Pd/ij6kBBYAYhKNDmi9Z4GDNyZJNs+hnU9fveYAP\n" \
    "6hBmf33GZYTeBbQRvTIDp+h4RF1BvBq49lHck88XW3QOXzhRbDr7c/0kTdSSNEsM\n" \
    "m6DWXoi9qmH6IbVV99jVaS+1H8pTGPbqKajA6fP+D+bWPUZt428SGyzpMYqLoW+i\n" \
    "SFW0/GSX3AeP4Q8C/f/hfxaRfnEXtCTetpZM0T9B1WFdUHc62z4TaGJnt8cy9NEb\n" \
    "QwIDAQABo1MwUTAdBgNVHQ4EFgQU2RwzPyzfmd08Z8bRgEsVwUDPng0wHwYDVR0j\n" \
    "BBgwFoAU2RwzPyzfmd08Z8bRgEsVwUDPng0wDwYDVR0TAQH/BAUwAwEB/zANBgkq\n" \
    "hkiG9w0BAQsFAAOCAQEAlbZUDoFR42MG0RsjvpvT5ZXjsXSOtXp5oOSWrr75XBdX\n" \
    "kRIQ7A0jWXac8/sZgSMNtqjBl4g+tkfMjVGbpBU+k7ZjJ1Hk+2ewftJpOktiJ9oR\n" \
    "dTnAfL6TWdL0q4VW1ua+miiv1IqkdreTfQ50UdutoQuptTG530Yx/pexA0XjMR0Q\n" \
    "ca880dF47gLCsX0uTdq7toR+C1ThetqlySpVeBRSqdkqOkBuhItsw8pkck8/6Kpz\n" \
    "nCz4x7Jx3re10eespYO1o5TA1uUsXvPkTjIP7hSQq6fNgrzpCn696D6J3rIZdNiL\n" \
    "7pNtJngjzvEjmjJEUlq52xgxlhIJ4F671r3cFzl6Ng==\n" \
    "-----END CERTIFICATE-----"

// ==================== MQTT 主题 ====================
#define TOPIC_RFID          "dorm/door/rfid"        // 发布: RFID刷卡事件(发UID给后端校验)
#define TOPIC_PIR           "dorm/door/pir"         // 发布: 人体红外检测
#define TOPIC_DHT           "dorm/door/dht"         // 发布: 温湿度数据
#define TOPIC_ALARM         "dorm/door/alarm"       // 发布: 告警事件(后端已自动生成, 保留备用)
#define TOPIC_CMD           "dorm/door/cmd"         // 订阅: 后端下发开门/拒绝指令

// ==================== 引脚定义 ====================
// MFRC522 SPI
#define PIN_RC522_MOSI      23
#define PIN_RC522_MISO      19
#define PIN_RC522_SCLK      18
#define PIN_RC522_CS        5
#define PIN_RC522_RST       22

// DHT11 温湿度
#define PIN_DHT11           16

// HC-SR501 人体红外
#define PIN_PIR             15

// 蜂鸣器 (有源，低电平触发，GPIO32=非启动引脚，不干扰烧录)
#define PIN_BUZZER          21

// ==================== 阈值 ====================
#define TEMP_MAX            40.0f   // 温度上限 (℃)
#define HUMIDITY_MAX        80.0f   // 湿度上限 (%)

// ==================== 时间间隔 (毫秒) ====================
#define DHT_INTERVAL_MS     2000    // DHT11 采集间隔 (2秒上报)
#define PIR_INTERVAL_MS     2000    // PIR 状态上报间隔 (2秒)
#define RFID_COOLDOWN_MS    2000    // 同卡读取冷却

// ==================== 舵机 ====================
#define PIN_SERVO           13

// ==================== 蜂鸣器 PWM 参数 ====================
#define BUZZER_PWM_FREQ     2000    // 蜂鸣器频率
#define BUZZER_PWM_TIMER    LEDC_TIMER_1
#define BUZZER_PWM_CHANNEL  LEDC_CHANNEL_1

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
