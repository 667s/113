/**
 * aes_crypto.c - ESP32 AES-128-CBC 加密模块实现
 *
 * AES-128-CBC 加密使用 mbedtls (ESP-IDF 内置)
 * Base64 编解码自实现 (避开 mbedtls 3.x API 兼容问题)
 * 随机 IV 使用 esp_random() (硬件随机源)
 *
 * 密文格式: Base64( IV(16字节) + AES密文 )
 * 对应后端: util/AesUtil.java
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/aes.h"
#include "aes_crypto.h"
#include "config.h"

static const char *TAG = "AES";

// AES-128 密钥 (16字节，从 hex 字符串解析)
static uint8_t aes_key[16] = {0};
static int aes_initialized = 0;

// ==================== Base64 编解码 (自实现) ====================

static const char B64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/** Base64 编码 (标准 RFC 4648) */
static int base64_encode(const uint8_t *src, size_t src_len,
                         char *dst, size_t dst_len)
{
    size_t out_len = ((src_len + 2) / 3) * 4;
    if (dst_len < out_len + 1) return -1;

    size_t j = 0;
    for (size_t i = 0; i < src_len; i += 3) {
        // 取出当前3字节 (不足补0)
        uint32_t val = (uint32_t)src[i] << 16;
        int have1 = (i + 1 < src_len);
        int have2 = (i + 2 < src_len);
        if (have1) val |= (uint32_t)src[i + 1] << 8;
        if (have2) val |= (uint32_t)src[i + 2];

        // 输出4个 Base64 字符
        dst[j++] = B64_TABLE[(val >> 18) & 0x3F];
        dst[j++] = B64_TABLE[(val >> 12) & 0x3F];
        dst[j++] = have1 ? B64_TABLE[(val >> 6) & 0x3F] : '=';
        dst[j++] = have2 ? B64_TABLE[val & 0x3F] : '=';
    }
    dst[j] = '\0';
    return j;
}

/** Base64 解码 */
static int base64_decode(const char *src, size_t src_len,
                         uint8_t *dst, size_t dst_len)
{
    // Lookup table to get index from base64 char
    static uint8_t lookup[128] = {0};
    static int lookup_init = 0;
    if (!lookup_init) {
        for (int i = 0; i < 64; i++) lookup[(int)B64_TABLE[i]] = i;
        lookup_init = 1;
    }

    size_t out_len = 0;
    uint32_t buf = 0;
    int bits = 0;

    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '=') break;
        if (src[i] == '\n' || src[i] == '\r') continue;

        uint8_t val = (uint8_t)src[i];
        if (val >= 128) continue;
        uint8_t idx = lookup[val];
        if (idx == 0 && src[i] != 'A') continue;  // not a valid b64 char

        buf = (buf << 6) | idx;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            if (out_len < dst_len) {
                dst[out_len] = (buf >> bits) & 0xFF;
            }
            out_len++;
        }
    }
    return out_len;
}

// ==================== AES 密钥初始化 ====================

static void aes_init_key(void)
{
    if (aes_initialized) return;

    const char *hex = AES_KEY_HEX;
    for (int i = 0; i < 16; i++) {
        unsigned int byte = 0;
        // 手动解析 hex (比 sscanf 更稳定)
        char hi = hex[i * 2];
        char lo = hex[i * 2 + 1];
        if (hi >= '0' && hi <= '9') byte |= (hi - '0') << 4;
        else if (hi >= 'a' && hi <= 'f') byte |= (hi - 'a' + 10) << 4;
        else if (hi >= 'A' && hi <= 'F') byte |= (hi - 'A' + 10) << 4;
        if (lo >= '0' && lo <= '9') byte |= (lo - '0');
        else if (lo >= 'a' && lo <= 'f') byte |= (lo - 'a' + 10);
        else if (lo >= 'A' && lo <= 'F') byte |= (lo - 'A' + 10);
        aes_key[i] = (uint8_t)byte;
    }
    aes_initialized = 1;
    ESP_LOGI(TAG, "AES key initialized (hex: %.32s)", AES_KEY_HEX);
}

// ==================== 随机 IV ====================

static void generate_iv(uint8_t *iv)
{
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    uint32_t r3 = esp_random();
    uint32_t r4 = esp_random();

    iv[0]  = (r1 >> 0)  & 0xFF;
    iv[1]  = (r1 >> 8)  & 0xFF;
    iv[2]  = (r1 >> 16) & 0xFF;
    iv[3]  = (r1 >> 24) & 0xFF;
    iv[4]  = (r2 >> 0)  & 0xFF;
    iv[5]  = (r2 >> 8)  & 0xFF;
    iv[6]  = (r2 >> 16) & 0xFF;
    iv[7]  = (r2 >> 24) & 0xFF;
    iv[8]  = (r3 >> 0)  & 0xFF;
    iv[9]  = (r3 >> 8)  & 0xFF;
    iv[10] = (r3 >> 16) & 0xFF;
    iv[11] = (r3 >> 24) & 0xFF;
    iv[12] = (r4 >> 0)  & 0xFF;
    iv[13] = (r4 >> 8)  & 0xFF;
    iv[14] = (r4 >> 16) & 0xFF;
    iv[15] = (r4 >> 24) & 0xFF;
}

// ==================== PKCS7 填充 ====================

static int pkcs7_pad(const uint8_t *input, size_t input_len,
                     uint8_t *output, size_t block_size)
{
    size_t pad_len = block_size - (input_len % block_size);
    memcpy(output, input, input_len);
    memset(output + input_len, (int)pad_len, pad_len);
    return input_len + pad_len;
}

// ==================== 公开 API ====================

char *aes_encrypt_to_base64(const char *plaintext)
{
    aes_init_key();

    // 1. 生成随机 IV (先保存，mbedtls 会修改它)
    uint8_t iv[16], saved_iv[16];
    generate_iv(iv);
    memcpy(saved_iv, iv, 16);

    // 2. PKCS7 填充
    size_t plain_len = strlen(plaintext);
    size_t padded_len = plain_len + 16;  // 最大填充一个完整块
    uint8_t *padded = (uint8_t *)malloc(padded_len);
    if (!padded) {
        ESP_LOGE(TAG, "malloc padded failed");
        return NULL;
    }
    padded_len = pkcs7_pad((const uint8_t *)plaintext, plain_len, padded, 16);

    // 3. AES-128-CBC 加密
    uint8_t *cipher = (uint8_t *)malloc(padded_len);
    if (!cipher) {
        ESP_LOGE(TAG, "malloc cipher failed");
        free(padded);
        return NULL;
    }

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, aes_key, 128);
    int aes_ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT,
                                        padded_len, iv,
                                        (const unsigned char *)padded,
                                        cipher);
    mbedtls_aes_free(&aes_ctx);
    free(padded);

    if (aes_ret != 0) {
        ESP_LOGE(TAG, "AES encrypt failed: %d", aes_ret);
        free(cipher);
        return NULL;
    }

    // 4. IV(16) + 密文 拼接 (用保存的原始IV)
    size_t combined_len = 16 + padded_len;
    uint8_t *combined = (uint8_t *)malloc(combined_len);
    if (!combined) {
        ESP_LOGE(TAG, "malloc combined failed");
        free(cipher);
        return NULL;
    }
    memcpy(combined, saved_iv, 16);
    memcpy(combined + 16, cipher, padded_len);
    free(cipher);

    // 5. Base64 编码 (自实现)
    size_t b64_max_len = ((combined_len + 2) / 3) * 4 + 1;
    char *b64_out = (char *)malloc(b64_max_len);
    if (!b64_out) {
        ESP_LOGE(TAG, "malloc b64_out failed");
        free(combined);
        return NULL;
    }

    int b64_len = base64_encode(combined, combined_len, b64_out, b64_max_len);
    free(combined);

    if (b64_len < 0) {
        ESP_LOGE(TAG, "Base64 encode failed");
        free(b64_out);
        return NULL;
    }

    ESP_LOGI(TAG, "AES encrypted: %zu bytes plain -> %d bytes base64",
             plain_len, b64_len);
    return b64_out;
}

char *aes_decrypt_from_base64(const char *base64_cipher)
{
    aes_init_key();

    // 1. Base64 解码 (自实现)
    size_t b64_len = strlen(base64_cipher);
    size_t decoded_max = ((b64_len + 3) / 4) * 3;
    uint8_t *decoded = (uint8_t *)malloc(decoded_max);
    if (!decoded) {
        ESP_LOGE(TAG, "malloc decoded failed");
        return NULL;
    }

    int decoded_len = base64_decode(base64_cipher, b64_len, decoded, decoded_max);
    if (decoded_len < 16) {
        ESP_LOGE(TAG, "Base64 decode too short: %d bytes", decoded_len);
        free(decoded);
        return NULL;
    }

    // 2. 提取 IV (前16字节) + 密文
    uint8_t iv[16];
    memcpy(iv, decoded, 16);
    size_t cipher_len = decoded_len - 16;

    // 3. AES-CBC 解密
    uint8_t *plain = (uint8_t *)malloc(cipher_len + 1);
    if (!plain) {
        ESP_LOGE(TAG, "malloc plain failed");
        free(decoded);
        return NULL;
    }

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_dec(&aes_ctx, aes_key, 128);
    int aes_ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT,
                                        cipher_len, iv,
                                        decoded + 16, plain);
    mbedtls_aes_free(&aes_ctx);
    free(decoded);

    if (aes_ret != 0) {
        ESP_LOGE(TAG, "AES decrypt failed: %d", aes_ret);
        free(plain);
        return NULL;
    }

    // 4. 去掉 PKCS7 填充
    uint8_t pad_len = plain[cipher_len - 1];
    if (pad_len > 0 && pad_len <= 16) {
        cipher_len -= pad_len;
    }
    plain[cipher_len] = '\0';

    return (char *)plain;
}
