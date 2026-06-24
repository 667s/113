/**
 * authorized_cards.h - 本地授权卡白名单
 *
 * ===== 如何获取卡号 UID =====
 * 1. 烧录代码后，打开串口监视器 (idf.py monitor)
 * 2. 刷一下卡，串口会打印类似:
 *    [RFID] Card UID: A3 7F 2C 10
 * 3. 把打印出来的 UID 替换到下面的列表里
 *
 * ===== UID 格式说明 =====
 * MIFARE Classic 1K 通常是 4 字节 UID
 * 格式: 两位十六进制大写字母，空格分隔
 * 例如: "A3 7F 2C 10"
 */

#ifndef AUTHORIZED_CARDS_H
#define AUTHORIZED_CARDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <strings.h>

// 授权卡最大数量
#define MAX_AUTHORIZED_CARDS 10

// 当前已注册的授权卡数量
#define AUTHORIZED_CARD_COUNT 3

// 卡片信息结构体
typedef struct {
    char uid[20];           // UID 字符串，如 "A3 7F 2C 10"
    char holder_name[32];   // 持卡人姓名
} authorized_card_t;

// ===== 授权卡列表 (请替换为你的实际卡号!) =====
static const authorized_card_t g_authorized_cards[AUTHORIZED_CARD_COUNT] = {
    { .uid = "48 92 0D E7", .holder_name = "我" },
    { .uid = "B4 8E 3D 21", .holder_name = "李四" },
    { .uid = "C5 9F 4E 32", .holder_name = "管理员" },
};

/**
 * 查找卡号是否在白名单中
 * @param uid_str  待查找的 UID 字符串
 * @param out_name 输出: 持卡人姓名（NULL表示不需要）
 * @return 0=未授权, 1=已授权
 */
static inline int is_authorized_card(const char *uid_str, const char **out_name)
{
    for (int i = 0; i < AUTHORIZED_CARD_COUNT; i++) {
        if (strcasecmp(uid_str, g_authorized_cards[i].uid) == 0) {
            if (out_name) {
                *out_name = g_authorized_cards[i].holder_name;
            }
            return 1;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // AUTHORIZED_CARDS_H
