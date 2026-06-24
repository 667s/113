package com.dormsecurity.controller;

import com.dormsecurity.handler.MqttInboundHandler;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.HashMap;
import java.util.Map;

/**
 * 温湿度 / PIR 实时数据接口 (从内存读取，不查数据库)
 */
@RestController
@RequestMapping("/api/sensor")
public class SensorDataController {

    /** 最新温湿度 */
    @GetMapping("/dht/latest")
    public ResponseEntity<Map<String, Object>> getLatestDht() {
        Map<String, Object> result = new HashMap<>();
        result.put("temperature", MqttInboundHandler.getTemp());
        result.put("humidity", MqttInboundHandler.getHumidity());
        result.put("recordedAt", MqttInboundHandler.getDhtTime());
        return ResponseEntity.ok(result);
    }

    /** PIR 最新状态 */
    @GetMapping("/pir/status")
    public ResponseEntity<Map<String, Object>> getPirStatus() {
        Map<String, Object> result = new HashMap<>();
        result.put("detected", MqttInboundHandler.isPir());
        result.put("lastTime", MqttInboundHandler.getPirTime());
        return ResponseEntity.ok(result);
    }
}
