package com.dormsecurity.controller;

import com.dormsecurity.entity.AlarmLog;
import com.dormsecurity.service.AlarmService;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/alarm")
public class AlarmController {

    private final AlarmService alarmService;

    public AlarmController(AlarmService alarmService) {
        this.alarmService = alarmService;
    }

    /**
     * 分页查询告警记录
     * GET /api/alarm/logs?page=0&size=20
     * GET /api/alarm/logs?page=0&size=20&type=PIR
     */
    @GetMapping("/logs")
    public ResponseEntity<Map<String, Object>> getLogs(
            @RequestParam(defaultValue = "0") int page,
            @RequestParam(defaultValue = "20") int size,
            @RequestParam(required = false) String type) {

        Page<AlarmLog> logPage;
        if (type != null && !type.isEmpty()) {
            logPage = alarmService.findByType(type, PageRequest.of(page, size));
        } else {
            logPage = alarmService.findAll(PageRequest.of(page, size));
        }

        Map<String, Object> result = new HashMap<>();
        result.put("content", logPage.getContent());
        result.put("totalElements", logPage.getTotalElements());
        result.put("totalPages", logPage.getTotalPages());
        result.put("currentPage", page);
        return ResponseEntity.ok(result);
    }

    /**
     * 最新10条告警
     * GET /api/alarm/logs/latest
     */
    @GetMapping("/logs/latest")
    public ResponseEntity<List<AlarmLog>> getLatest() {
        return ResponseEntity.ok(alarmService.findLatest());
    }

    /**
     * 按时间范围查询告警
     * GET /api/alarm/logs/range?start=...&end=...
     */
    @GetMapping("/logs/range")
    public ResponseEntity<List<AlarmLog>> getByRange(
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime start,
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime end) {
        return ResponseEntity.ok(alarmService.findByTimeRange(start, end));
    }
}
