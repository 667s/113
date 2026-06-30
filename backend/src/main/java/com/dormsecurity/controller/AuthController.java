package com.dormsecurity.controller;

import com.dormsecurity.entity.User;
import com.dormsecurity.service.UserService;
import com.dormsecurity.util.JwtUtil;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;

@RestController
@RequestMapping("/api/auth")
public class AuthController {

    private final UserService userService;
    private final JwtUtil jwtUtil;

    public AuthController(UserService userService, JwtUtil jwtUtil) {
        this.userService = userService;
        this.jwtUtil = jwtUtil;
    }

    /**
     * 注册
     * POST /api/auth/register
     * Body: { "username": "...", "password": "...", "displayName": "..." }
     */
    @PostMapping("/register")
    public ResponseEntity<Map<String, Object>> register(@RequestBody Map<String, String> body) {
        String username = body.get("username");
        String password = body.get("password");
        String displayName = body.getOrDefault("displayName", username);

        // 参数校验
        if (username == null || username.trim().isEmpty() ||
            password == null || password.trim().isEmpty()) {
            Map<String, Object> err = new HashMap<>();
            err.put("error", "用户名和密码不能为空");
            return ResponseEntity.badRequest().body(err);
        }

        if (password.length() < 6) {
            Map<String, Object> err = new HashMap<>();
            err.put("error", "密码长度不能少于6位");
            return ResponseEntity.badRequest().body(err);
        }

        try {
            User user = userService.register(username.trim(), password, displayName.trim());
            Map<String, Object> result = new HashMap<>();
            result.put("id", user.getId());
            result.put("username", user.getUsername());
            result.put("displayName", user.getDisplayName());
            result.put("role", user.getRole());
            result.put("message", "注册成功");
            return ResponseEntity.ok(result);
        } catch (RuntimeException e) {
            Map<String, Object> err = new HashMap<>();
            err.put("error", e.getMessage());
            return ResponseEntity.badRequest().body(err);
        }
    }

    /**
     * 登录
     * POST /api/auth/login
     * Body: { "username": "...", "password": "..." }
     */
    @PostMapping("/login")
    public ResponseEntity<Map<String, Object>> login(@RequestBody Map<String, String> body) {
        String username = body.get("username");
        String password = body.get("password");

        if (username == null || password == null) {
            Map<String, Object> err = new HashMap<>();
            err.put("error", "用户名和密码不能为空");
            return ResponseEntity.badRequest().body(err);
        }

        User user = userService.authenticate(username, password);
        if (user == null) {
            Map<String, Object> err = new HashMap<>();
            err.put("error", "用户名或密码错误");
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED).body(err);
        }

        String token = jwtUtil.generateToken(user.getUsername(), user.getRole());

        Map<String, Object> result = new HashMap<>();
        result.put("token", token);
        result.put("username", user.getUsername());
        result.put("displayName", user.getDisplayName());
        result.put("role", user.getRole());
        return ResponseEntity.ok(result);
    }
}
