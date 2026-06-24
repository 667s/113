package com.dormsecurity.controller;

import com.dormsecurity.entity.RfidRecord;
import com.dormsecurity.service.RfidService;
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
@RequestMapping("/api/rfid")
public class RfidController {

    private final RfidService rfidService;

    public RfidController(RfidService rfidService) {
        this.rfidService = rfidService;
    }

    /**
     * 分页查询刷卡记录
     * GET /api/rfid/records?page=0&size=20
     */
    @GetMapping("/records")
    public ResponseEntity<Map<String, Object>> getRecords(
            @RequestParam(defaultValue = "0") int page,
            @RequestParam(defaultValue = "20") int size) {

        Page<RfidRecord> recordPage = rfidService.findAll(PageRequest.of(page, size));

        Map<String, Object> result = new HashMap<>();
        result.put("content", recordPage.getContent());
        result.put("totalElements", recordPage.getTotalElements());
        result.put("totalPages", recordPage.getTotalPages());
        result.put("currentPage", page);
        return ResponseEntity.ok(result);
    }

    /**
     * 最新10条刷卡记录
     * GET /api/rfid/records/latest
     */
    @GetMapping("/records/latest")
    public ResponseEntity<List<RfidRecord>> getLatest() {
        return ResponseEntity.ok(rfidService.findLatest());
    }

    /**
     * 按时间范围查询
     * GET /api/rfid/records/range?start=2024-01-01T00:00:00&end=2024-12-31T23:59:59
     */
    @GetMapping("/records/range")
    public ResponseEntity<List<RfidRecord>> getByRange(
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime start,
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime end) {
        return ResponseEntity.ok(rfidService.findByTimeRange(start, end));
    }
}
