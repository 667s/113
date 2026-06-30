package com.dormsecurity.controller;

import com.dormsecurity.service.ThresholdService;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;

/**
 * 阈值管理接口
 * GET  /api/threshold        — 获取当前阈值 (登录用户均可查看)
 * PUT  /api/threshold        — 更新阈值 (仅管理员)
 */
@RestController
@RequestMapping("/api/threshold")
public class ThresholdController {

    private final ThresholdService thresholdService;

    public ThresholdController(ThresholdService thresholdService) {
        this.thresholdService = thresholdService;
    }

    /** 获取阈值 */
    @GetMapping
    @PreAuthorize("hasAnyRole('ADMIN','USER')")
    public ResponseEntity<Map<String, Object>> getAll() {
        Map<String, String> thresholds = thresholdService.getAll();
        Map<String, Object> result = new HashMap<>();
        result.put("temp_max", thresholds.get("temp_max"));
        result.put("humidity_max", thresholds.get("humidity_max"));
        return ResponseEntity.ok(result);
    }

    /** 更新阈值（仅管理员） */
    @PutMapping
    @PreAuthorize("hasRole('ADMIN')")
    public ResponseEntity<Map<String, Object>> update(@RequestBody Map<String, String> body) {
        Map<String, Object> result = new HashMap<>();
        try {
            if (body.containsKey("temp_max")) {
                thresholdService.update("temp_max", body.get("temp_max"));
            }
            if (body.containsKey("humidity_max")) {
                thresholdService.update("humidity_max", body.get("humidity_max"));
            }
            result.put("message", "阈值更新成功");
            result.put("temp_max", thresholdService.getAll().get("temp_max"));
            result.put("humidity_max", thresholdService.getAll().get("humidity_max"));
            return ResponseEntity.ok(result);
        } catch (Exception e) {
            result.put("error", "更新失败: " + e.getMessage());
            return ResponseEntity.badRequest().body(result);
        }
    }
}
