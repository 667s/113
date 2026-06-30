package com.dormsecurity.service;

import com.dormsecurity.entity.User;
import com.dormsecurity.repository.UserRepository;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Service;

import java.util.Optional;

@Service
public class UserService {

    private final UserRepository userRepository;
    private final BCryptPasswordEncoder passwordEncoder = new BCryptPasswordEncoder();

    public UserService(UserRepository userRepository) {
        this.userRepository = userRepository;
    }

    /**
     * 注册新用户
     * @return 保存后的用户
     * @throws RuntimeException 用户名已存在
     */
    public User register(String username, String password, String displayName) {
        if (userRepository.existsByUsername(username)) {
            throw new RuntimeException("用户名已存在");
        }

        User user = new User();
        user.setUsername(username);
        user.setPassword(passwordEncoder.encode(password));  // BCrypt 加密
        user.setDisplayName(displayName != null ? displayName : username);
        user.setRole("ROLE_USER");  // 默认普通用户

        return userRepository.save(user);
    }

    /**
     * 登录验证 — 验证用户名密码
     * @return 验证成功返回 User，失败返回 null
     */
    public User authenticate(String username, String rawPassword) {
        Optional<User> opt = userRepository.findByUsername(username);
        if (opt.isEmpty()) {
            return null;
        }
        User user = opt.get();
        if (passwordEncoder.matches(rawPassword, user.getPassword())) {
            return user;
        }
        return null;
    }

    /** 按用户名查找 */
    public Optional<User> findByUsername(String username) {
        return userRepository.findByUsername(username);
    }
}
