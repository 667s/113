package com.dormsecurity.controller;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.http.ResponseEntity;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.support.MessageBuilder;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

/**
 * 远程控制接口 — 开门指令下发
 * POST /api/remote/open → MQTT dorm/door/cmd {"cmd":"open","uid":"remote","authorized":true}
 */
@RestController
@RequestMapping("/api/remote")
public class RemoteController {

    private final MessageChannel mqttOutputChannel;

    public RemoteController(@Qualifier("mqttOutputChannel") MessageChannel mqttOutputChannel) {
        this.mqttOutputChannel = mqttOutputChannel;
    }

    /**
     * 远程开门 (所有登录用户均可操作)
     */
    @PostMapping("/open")
    @PreAuthorize("hasAnyRole('ADMIN','USER')")
    public ResponseEntity<Map<String, Object>> openDoor() {
        String payload = "{\"cmd\":\"open\",\"uid\":\"remote\",\"authorized\":true}";
        mqttOutputChannel.send(MessageBuilder.withPayload(payload)
                .setHeader("mqtt_topic", "dorm/door/cmd")
                .build());

        return ResponseEntity.ok(Map.of(
            "message", "开门指令已发送"
        ));
    }
}
