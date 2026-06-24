package com.dormsecurity.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.server.standard.ServerEndpointExporter;

/**
 * WebSocket 配置
 * 用于向后端管理页面实时推送告警（可选）
 *
 * 鸿蒙前端主要通过 REST API 轮询 + MQTT 直接订阅获取实时告警
 * WebSocket 作为备用的服务端推送方案
 */
@Configuration
@EnableWebSocket
public class WebSocketConfig {

    @Bean
    public ServerEndpointExporter serverEndpointExporter() {
        return new ServerEndpointExporter();
    }
}
