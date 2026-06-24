/**
 * rfid_reader.c - MFRC522 RFID 读卡模块 (基于已验证的 rc522.c 参考代码)
 *
 * MFRC522 是一款 13.56MHz 非接触式读写芯片，通过 SPI 与 MCU 通信。
 * 支持: MIFARE Classic 1K/4K, MIFARE Ultralight
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"
#include "rfid_reader.h"

static const char *TAG = "RFID";

// ==================== MFRC522 寄存器地址 ====================
#define RC522_REG_COMMAND       0x01
#define RC522_REG_COM_IRQ       0x04
#define RC522_REG_ERROR         0x06
#define RC522_REG_FIFO_DATA     0x09
#define RC522_REG_FIFO_LEVEL    0x0A
#define RC522_REG_BIT_FRAMING   0x0D
#define RC522_REG_MODE          0x11
#define RC522_REG_TX_CONTROL    0x14
#define RC522_REG_TX_ASK        0x15
#define RC522_REG_T_MODE        0x2A
#define RC522_REG_T_PRESCALER   0x2B
#define RC522_REG_T_RELOAD_H    0x2C
#define RC522_REG_T_RELOAD_L    0x2D

// ==================== MFRC522 命令 ====================
#define RC522_CMD_IDLE          0x00
#define RC522_CMD_TRANSCEIVE    0x0C
#define RC522_CMD_SOFTRESET     0x0F

// ==================== PICC 命令 ====================
#define PICC_CMD_REQA           0x26
#define PICC_CMD_WUPA           0x52
#define PICC_CMD_ANTICOLL       0x93
#define PICC_ANTICOLL_LEVEL1    0x20
#define PICC_CMD_SELECT         0x93
#define PICC_CMD_HALT           0x50

// ==================== 状态标志 ====================
#define RC522_IRQ_RX_DONE       (1 << 5)
#define RC522_IRQ_IDLE          (1 << 4)
#define RC522_IRQ_TIMER         (1 << 0)

// ==================== 全局变量 ====================
static spi_device_handle_t s_spi = NULL;
static uint8_t             s_last_uid_bytes[10];
static uint8_t             s_last_uid_len = 0;
static int64_t             s_last_read_time_us = 0;

// ==================== 底层 SPI 操作 ====================

/** 写一个寄存器 */
static esp_err_t rc522_write_reg(uint8_t reg, uint8_t value)
{
    esp_err_t ret;
    spi_transaction_t t = {0};
    uint8_t addr = ((reg << 1) & 0x7E);  /* 写模式: bit[0]=0 */
    t.length = 16;
    t.tx_buffer = NULL;
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = addr;
    t.tx_data[1] = value;

    ret = spi_device_transmit(s_spi, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write reg 0x%02X failed", reg);
    }
    return ret;
}

/** 读一个寄存器 */
static uint8_t rc522_read_reg(uint8_t reg)
{
    esp_err_t ret;
    spi_transaction_t t = {0};
    uint8_t addr = 0x80 | ((reg << 1) & 0x7E);  /* 读模式: bit[0]=1 */
    t.length = 16;
    t.rxlength = 16;
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.tx_data[0] = addr;
    t.tx_data[1] = 0x00;

    ret = spi_device_transmit(s_spi, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read reg 0x%02X failed", reg);
        return 0;
    }
    return t.rx_data[1];
}

/** 软复位 (参考实现) */
static esp_err_t rc522_soft_reset(void)
{
    esp_err_t ret = rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFTRESET);
    if (ret != ESP_OK) return ret;
    uint8_t cmd;
    int timeout = 100;
    do {
        vTaskDelay(pdMS_TO_TICKS(1));
        cmd = rc522_read_reg(RC522_REG_COMMAND);
    } while ((cmd & 0x10) && --timeout);
    if (timeout == 0) {
        ESP_LOGE(TAG, "Soft reset timeout");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

// ==================== PICC 卡片操作 ====================

/**
 * 发送数据到卡片并接收应答 (完全复制参考 rc522.c 实现)
 */
static esp_err_t rc522_transceive(uint8_t *send_data, uint8_t send_len,
                                   uint8_t *recv_data, uint8_t *recv_len)
{
    /* 1. Flush FIFO */
    rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80);

    /* 2. 写入发送数据到 FIFO (逐字节) */
    for (int i = 0; i < send_len; i++) {
        rc522_write_reg(RC522_REG_FIFO_DATA, send_data[i]);
    }

    /* 3. 清除所有中断标志 */
    rc522_write_reg(RC522_REG_COM_IRQ, 0x7F);

    /* 4. 执行 Transceive 命令 */
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);

    /* 5. StartSend: 必须在命令之后设置 bit7 */
    uint8_t framing = rc522_read_reg(RC522_REG_BIT_FRAMING);
    rc522_write_reg(RC522_REG_BIT_FRAMING, framing | 0x80);

    /* 6. 等待中断: RxIRq(0x20) | IdleIRq(0x10) | TimerIRq(0x01) */
    uint8_t irq = 0;
    int timeout = 600;
    do {
        vTaskDelay(pdMS_TO_TICKS(1));
        irq = rc522_read_reg(RC522_REG_COM_IRQ);
    } while (!(irq & 0x31) && --timeout);

    if (timeout == 0) {
        return ESP_ERR_TIMEOUT;
    }

    /* 7. 检查错误: WrErr(0x10) 或 BufferOvfl(0x40) */
    uint8_t error = rc522_read_reg(RC522_REG_ERROR);
    if (error & 0x50) {
        ESP_LOGW(TAG, "Transceive HW error: 0x%02X", error);
        return ESP_FAIL;
    }

    /* 8. 读取接收到的数据 (逐字节) */
    uint8_t fifo_level = rc522_read_reg(RC522_REG_FIFO_LEVEL);

    for (int i = 0; i < fifo_level && i < *recv_len; i++) {
        recv_data[i] = rc522_read_reg(RC522_REG_FIFO_DATA);
    }
    *recv_len = (fifo_level < *recv_len) ? fifo_level : *recv_len;

    return ESP_OK;
}

/** 寻卡 (REQA / WUPA) — 完全复制参考实现 */
static esp_err_t rc522_request(uint8_t req_mode)
{
    uint8_t send = req_mode;
    uint8_t recv[2] = {0};
    uint8_t recv_len = sizeof(recv);
    esp_err_t ret;

    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x07);  /* 7-bit short frame */
    ret = rc522_transceive(&send, 1, recv, &recv_len);
    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x00);  /* back to normal */

    if (ret != ESP_OK) return ret;
    if (recv_len != 2) return ESP_ERR_NOT_FOUND;

    return ESP_OK;
}

/** 防冲突获取 UID (跳过 Select 避免 CRC) — 完全复制参考实现 */
static esp_err_t rc522_anticoll_get_uid(uint8_t *uid, uint8_t *uid_len)
{
    esp_err_t ret;
    uint8_t send[2], recv[10];
    uint8_t recv_len;

    send[0] = PICC_CMD_ANTICOLL;       /* 0x93 */
    send[1] = PICC_ANTICOLL_LEVEL1;    /* 0x20 */
    recv_len = sizeof(recv);
    memset(recv, 0, recv_len);

    ret = rc522_transceive(send, 2, recv, &recv_len);
    if (ret != ESP_OK) return ret;

    if (recv_len < 5) return ESP_FAIL;

    /* 校验 BCC: XOR of UID[0..3] + BCC == 0 */
    if ((recv[0] ^ recv[1] ^ recv[2] ^ recv[3] ^ recv[4]) != 0) return ESP_FAIL;

    memcpy(uid, recv, 4);
    *uid_len = 4;
    return ESP_OK;
}

// ==================== 公开 API ====================

esp_err_t rc522_init(void)
{
    esp_err_t ret;
    uint8_t ver;

    ESP_LOGI(TAG, "Initializing MFRC522 RFID reader on SPI2...");

    /* 1. 配置 RST 引脚 (GPIO 输出，上拉) */
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << PIN_RC522_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_conf);
    gpio_set_level(PIN_RC522_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 2. SPI 总线初始化 */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_RC522_MOSI,
        .miso_io_num = PIN_RC522_MISO,
        .sclk_io_num = PIN_RC522_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init FAILED");
        return ret;
    }

    /* 3. 添加 SPI 设备 */
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 1 * 1000 * 1000,  /* 1 MHz */
        .spics_io_num = PIN_RC522_CS,
        .queue_size = 7,
    };
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device FAILED");
        return ret;
    }

    /* 4. 硬件复位: RST 脉冲低 */
    gpio_set_level(PIN_RC522_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RC522_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 5. 软复位 */
    ret = rc522_soft_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Soft reset FAILED");
        return ret;
    }

    /* 6. 验证版本号 */
    ver = rc522_read_reg(0x37);
    ESP_LOGI(TAG, "VersionReg = 0x%02X (expected 0x92 or 0x91)", ver);
    if (ver != 0x92 && ver != 0x91 && ver != 0x90) {
        ESP_LOGW(TAG, "Unexpected version! Check wiring");
    }

    /* 7. 配置定时器 */
    rc522_write_reg(RC522_REG_T_MODE, 0x80);
    rc522_write_reg(RC522_REG_T_PRESCALER, 0xA9);
    rc522_write_reg(RC522_REG_T_RELOAD_H, 0x03);
    rc522_write_reg(RC522_REG_T_RELOAD_L, 0xE8);

    /* 8. 调制和 CRC 模式 */
    rc522_write_reg(RC522_REG_TX_ASK, 0x40);
    rc522_write_reg(RC522_REG_MODE, 0x3D);

    /* 9. 开启天线 + RxGain 最大 */
    uint8_t tx_ctrl = rc522_read_reg(RC522_REG_TX_CONTROL);
    rc522_write_reg(RC522_REG_TX_CONTROL, tx_ctrl | 0x03);
    rc522_write_reg(0x26, 0x70);  /* RF_CFG: RxGain max */

    ESP_LOGI(TAG, "MFRC522 initialized successfully");
    return ESP_OK;
}

esp_err_t rc522_read_card(rfid_result_t *result)
{
    memset(result, 0, sizeof(rfid_result_t));

    /* 1. REQA 寻卡 */
    esp_err_t ret = rc522_request(PICC_CMD_REQA);
    if (ret != ESP_OK) {
        /* 无卡 → 正常返回 (高频轮询) */
        return ESP_OK;
    }

    /* 2. 防冲突获取 UID */
    uint8_t uid[4] = {0};
    uint8_t uid_len = 0;
    ret = rc522_anticoll_get_uid(uid, &uid_len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Anticollision failed");
        return ESP_OK;
    }

    /* 3. 防重复读取 (cooldown) */
    int is_same = (uid_len == s_last_uid_len) &&
                  (memcmp(uid, s_last_uid_bytes, uid_len) == 0);
    int64_t now_us = esp_timer_get_time();
    if (is_same && (now_us - s_last_read_time_us) < (RFID_COOLDOWN_MS * 1000)) {
        return ESP_OK;
    }
    memcpy(s_last_uid_bytes, uid, uid_len);
    s_last_uid_len = uid_len;
    s_last_read_time_us = now_us;

    /* 4. 填充结果 */
    result->has_card = 1;
    result->uid_len = uid_len;
    memcpy(result->uid_bytes, uid, uid_len);

    /* 格式化 UID 字符串 (连续无空格) */
    int pos = 0;
    for (int i = 0; i < uid_len; i++) {
        if (i > 0) result->uid[pos++] = ' ';
        pos += sprintf(result->uid + pos, "%02X", uid[i]);
    }

    /* 5. 授权状态由 HTTP 查询后端决定，此处不判定 */
    result->is_authorized = 0;  // 先默认未授权，handle_rfid 中 HTTP 查后端覆盖

    ESP_LOGI(TAG, "Card UID: %s (auth check by backend)", result->uid);

    /* 6. 休眠卡片 */
    rc522_halt();

    return ESP_OK;
}

esp_err_t rc522_halt(void)
{
    uint8_t halt_cmd[] = {PICC_CMD_HALT, 0x00};
    uint8_t dummy[4];
    uint8_t dl = sizeof(dummy);
    rc522_transceive(halt_cmd, 2, dummy, &dl);
    return ESP_OK;
}
