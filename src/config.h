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

// ==================== EMQX MQTT 配置 ====================
#define MQTT_BROKER_URI     "mqtt://10.160.153.147:1883"
#define MQTT_CLIENT_ID      "esp32_dorm_door"

// ==================== MQTT 主题 ====================
#define TOPIC_RFID          "dorm/door/rfid"        // RFID刷卡事件
#define TOPIC_PIR           "dorm/door/pir"         // 人体红外检测
#define TOPIC_DHT           "dorm/door/dht"         // 温湿度数据
#define TOPIC_ALARM         "dorm/door/alarm"       // 告警事件
#define TOPIC_SERVO_CMD     "dorm/door/servo/cmd"   // 远程开门指令(订阅)

// ==================== 引脚定义 ====================
// MFRC522 SPI
#define PIN_RC522_MOSI      23
#define PIN_RC522_MISO      19
#define PIN_RC522_SCLK      18
#define PIN_RC522_CS        5
#define PIN_RC522_RST       22

// DHT11 温湿度
#define PIN_DHT11           4

// HC-SR501 人体红外
#define PIN_PIR             15

// 蜂鸣器 (共用)
#define PIN_BUZZER          2

// SG90 舵机
#define PIN_SERVO           13

// ==================== 阈值 ====================
#define TEMP_MAX            40.0f   // 温度上限 (℃)
#define HUMIDITY_MAX        80.0f   // 湿度上限 (%)

// ==================== 时间间隔 (毫秒) ====================
#define DHT_INTERVAL_MS     30000   // DHT11 采集间隔
#define PIR_COOLDOWN_MS     5000    // PIR 冷却时间
#define RFID_COOLDOWN_MS    2000    // 同卡读取冷却

// ==================== 舵机角度 ====================
#define SERVO_OPEN_DEG      90      // 开门角度
#define SERVO_CLOSE_DEG     0       // 关门角度

// ==================== LEDC 参数 (舵机 PWM) ====================
#define SERVO_PWM_FREQ      50      // SG90 标准频率 50Hz
#define SERVO_PWM_TIMER     LEDC_TIMER_0
#define SERVO_PWM_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_PWM_CHANNEL   LEDC_CHANNEL_0

// ==================== 蜂鸣器 PWM 参数 ====================
#define BUZZER_PWM_FREQ     2000    // 蜂鸣器频率
#define BUZZER_PWM_TIMER    LEDC_TIMER_1
#define BUZZER_PWM_CHANNEL  LEDC_CHANNEL_1

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
