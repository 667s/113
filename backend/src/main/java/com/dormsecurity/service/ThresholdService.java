package com.dormsecurity.service;

import com.dormsecurity.entity.ThresholdConfig;
import com.dormsecurity.repository.ThresholdConfigRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.CommandLineRunner;
import org.springframework.stereotype.Service;

import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 阈值服务 — 内存缓存 + 数据库存储
 */
@Service
public class ThresholdService implements CommandLineRunner {

    private static final Logger log = LoggerFactory.getLogger(ThresholdService.class);

    private final ThresholdConfigRepository repository;

    // 内存缓存（避免每次查库）
    private final Map<String, String> cache = new ConcurrentHashMap<>();

    public ThresholdService(ThresholdConfigRepository repository) {
        this.repository = repository;
    }

    /** 启动时加载默认值 */
    @Override
    public void run(String... args) {
        initDefault("temp_max", "40.0");
        initDefault("humidity_max", "80.0");
        log.info("阈值已加载: temp_max={}, humidity_max={}",
                 getValue("temp_max"), getValue("humidity_max"));
    }

    private void initDefault(String key, String defaultValue) {
        if (repository.findByConfigKey(key).isEmpty()) {
            repository.save(new ThresholdConfig(key, defaultValue));
        }
        cache.put(key, getDbValue(key, defaultValue));
    }

    /** 获取阈值（float） */
    public float getFloat(String key) {
        try {
            return Float.parseFloat(getValue(key));
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    /** 获取所有阈值 */
    public Map<String, String> getAll() {
        Map<String, String> result = new ConcurrentHashMap<>();
        result.put("temp_max", getValue("temp_max"));
        result.put("humidity_max", getValue("humidity_max"));
        return result;
    }

    /** 更新阈值 */
    public void update(String key, String value) {
        Optional<ThresholdConfig> opt = repository.findByConfigKey(key);
        ThresholdConfig config;
        if (opt.isPresent()) {
            config = opt.get();
            config.setConfigValue(value);
        } else {
            config = new ThresholdConfig(key, value);
        }
        repository.save(config);
        cache.put(key, value);
        log.info("阈值更新: {} = {}", key, value);
    }

    private String getValue(String key) {
        return cache.computeIfAbsent(key, k -> getDbValue(k, "0"));
    }

    private String getDbValue(String key, String fallback) {
        return repository.findByConfigKey(key)
                .map(ThresholdConfig::getConfigValue)
                .orElse(fallback);
    }
}
