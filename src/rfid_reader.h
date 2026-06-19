/**
 * rfid_reader.h - MFRC522 RFID 读卡模块 (原生 ESP-IDF SPI 驱动)
 *
 * 支持的卡片: MIFARE Classic 1K/4K, MIFARE Ultralight
 * 通信接口: SPI (最多 10 Mbps)
 */

#ifndef RFID_READER_H
#define RFID_READER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"

/** 最大 UID 长度 (7字节 + 空格分隔) */
#define RFID_MAX_UID_LEN    32

/** 刷卡结果 */
typedef struct {
    int     has_card;                       // 是否检测到卡
    char    uid[RFID_MAX_UID_LEN];          // UID 字符串 (空格分隔)
    uint8_t uid_bytes[10];                  // UID 原始字节
    uint8_t uid_len;                        // UID 字节数
    int     is_authorized;                  // 是否授权
    char    holder_name[32];                // 持卡人姓名
} rfid_result_t;

/**
 * 初始化 MFRC522
 * - 配置 SPI 总线
 * - 软复位芯片
 * - 设置定时器
 * - 开启天线
 */
esp_err_t rc522_init(void);

/**
 * 检查并读取卡片 (非阻塞)
 * 包含: 寻卡 → 防冲突 → 选卡 → 白名单比对
 *
 * @param result 输出: 刷卡结果
 * @return ESP_OK
 */
esp_err_t rc522_read_card(rfid_result_t *result);

/**
 * 停止当前卡片通信 (Halt)
 */
esp_err_t rc522_halt(void);

#ifdef __cplusplus
}
#endif

#endif // RFID_READER_H
