package com.dormsecurity.entity;

import jakarta.persistence.*;

/**
 * 阈值配置实体 — key-value 存储
 * 如: temp_max = 40.0, humidity_max = 80.0
 */
@Entity
@Table(name = "threshold_config")
public class ThresholdConfig {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(unique = true, nullable = false, length = 50)
    private String configKey;

    @Column(nullable = false, length = 100)
    private String configValue;

    public ThresholdConfig() {}

    public ThresholdConfig(String configKey, String configValue) {
        this.configKey = configKey;
        this.configValue = configValue;
    }

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getConfigKey() { return configKey; }
    public void setConfigKey(String configKey) { this.configKey = configKey; }

    public String getConfigValue() { return configValue; }
    public void setConfigValue(String configValue) { this.configValue = configValue; }
}
