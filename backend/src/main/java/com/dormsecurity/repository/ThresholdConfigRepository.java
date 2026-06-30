package com.dormsecurity.repository;

import com.dormsecurity.entity.ThresholdConfig;
import org.springframework.data.jpa.repository.JpaRepository;
import java.util.Optional;

public interface ThresholdConfigRepository extends JpaRepository<ThresholdConfig, Long> {
    Optional<ThresholdConfig> findByConfigKey(String configKey);
}
