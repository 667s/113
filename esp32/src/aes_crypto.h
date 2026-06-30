/**
 * aes_crypto.h - ESP32 AES-128-CBC 加密模块
 *
 * 基于 ESP-IDF 内置 mbedtls 库
 * 密文格式: Base64( IV(16字节) + 加密数据 )
 */
#ifndef AES_CRYPTO_H
#define AES_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AES-128-CBC 加密 + Base64 编码
 * @param plaintext 明文字符串
 * @return Base64 密文 (malloc, 调用者负责 free)
 */
char *aes_encrypt_to_base64(const char *plaintext);

/**
 * AES-128-CBC 解密 Base64 密文
 * @param base64_cipher Base64 编码的密文
 * @return 明文字符串 (malloc, 调用者负责 free)
 */
char *aes_decrypt_from_base64(const char *base64_cipher);

#ifdef __cplusplus
}
#endif

#endif // AES_CRYPTO_H
