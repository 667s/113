package com.dormsecurity.util;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.Base64;

/**
 * AES-128-CBC 加解密工具
 * 对应 ESP32 端 mbedtls AES 加密
 *
 * 密文格式: Base64( IV(16字节) + 加密数据 )
 */
@Component
public class AesUtil {

    private static final String ALGORITHM = "AES/CBC/PKCS5Padding";
    private static final int IV_LENGTH = 16;

    private final SecretKeySpec keySpec;

    public AesUtil(@Value("${aes.key}") String hexKey) {
        byte[] keyBytes = hexStringToBytes(hexKey);
        this.keySpec = new SecretKeySpec(keyBytes, "AES");
    }

    /**
     * AES 解密 Base64 密文
     * @param base64Cipher Base64( IV + 密文 )
     * @return 明文字符串
     */
    public String decrypt(String base64Cipher) throws Exception {
        byte[] combined = Base64.getDecoder().decode(base64Cipher);

        // 提取 IV (前16字节)
        byte[] iv = new byte[IV_LENGTH];
        System.arraycopy(combined, 0, iv, 0, IV_LENGTH);

        // 提取密文
        byte[] cipherBytes = new byte[combined.length - IV_LENGTH];
        System.arraycopy(combined, IV_LENGTH, cipherBytes, 0, cipherBytes.length);

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        cipher.init(Cipher.DECRYPT_MODE, keySpec, new IvParameterSpec(iv));

        byte[] plainBytes = cipher.doFinal(cipherBytes);
        return new String(plainBytes, StandardCharsets.UTF_8);
    }

    /**
     * AES 加密 (用于调试/指令下发)
     * @return Base64( IV + 密文 )
     */
    public String encrypt(String plaintext) throws Exception {
        byte[] iv = new byte[IV_LENGTH];
        new SecureRandom().nextBytes(iv);

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        cipher.init(Cipher.ENCRYPT_MODE, keySpec, new IvParameterSpec(iv));

        byte[] cipherBytes = cipher.doFinal(plaintext.getBytes(StandardCharsets.UTF_8));

        // IV + 密文 拼接后再 Base64
        byte[] combined = new byte[IV_LENGTH + cipherBytes.length];
        System.arraycopy(iv, 0, combined, 0, IV_LENGTH);
        System.arraycopy(cipherBytes, 0, combined, IV_LENGTH, cipherBytes.length);

        return Base64.getEncoder().encodeToString(combined);
    }

    private static byte[] hexStringToBytes(String hex) {
        int len = hex.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(hex.charAt(i), 16) << 4)
                    + Character.digit(hex.charAt(i + 1), 16));
        }
        return data;
    }
}
