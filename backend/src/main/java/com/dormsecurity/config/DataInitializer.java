package com.dormsecurity.config;

import com.dormsecurity.entity.User;
import com.dormsecurity.repository.UserRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.CommandLineRunner;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Component;

/**
 * 数据初始化器 — 预置管理员账号
 */
@Component
public class DataInitializer implements CommandLineRunner {

    private static final Logger log = LoggerFactory.getLogger(DataInitializer.class);

    private final UserRepository userRepository;
    private final BCryptPasswordEncoder passwordEncoder;

    public DataInitializer(UserRepository userRepository, BCryptPasswordEncoder passwordEncoder) {
        this.userRepository = userRepository;
        this.passwordEncoder = passwordEncoder;
    }

    @Override
    public void run(String... args) {
        String adminUsername = "2340711113";

        if (userRepository.existsByUsername(adminUsername)) {
            log.info("管理员账号已存在: {}", adminUsername);
            return;
        }

        User admin = new User();
        admin.setUsername(adminUsername);
        admin.setPassword(passwordEncoder.encode(adminUsername));
        admin.setDisplayName("管理员");
        admin.setRole("ROLE_ADMIN");
        userRepository.save(admin);

        log.info("===========================================");
        log.info("  管理员账号已创建!");
        log.info("  用户名: {}", adminUsername);
        log.info("  密码:   {}", adminUsername);
        log.info("  角色:   ROLE_ADMIN");
        log.info("===========================================");
    }
}
