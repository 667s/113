package com.dormsecurity.handler;

import com.dormsecurity.entity.AlarmLog;
import com.dormsecurity.entity.RfidRecord;
import com.dormsecurity.service.AlarmService;
import com.dormsecurity.service.AuthorizedCardService;
import com.dormsecurity.service.RfidService;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.messaging.Message;
import org.springframework.messaging.MessageHandler;
import org.springframework.messaging.MessagingException;
import java.time.LocalDateTime;

public class MqttInboundHandler implements MessageHandler {

    private static final Logger log = LoggerFactory.getLogger(MqttInboundHandler.class);
    private final ObjectMapper objectMapper = new ObjectMapper();

    private static RfidService rfidService;
    private static AlarmService alarmService;
    private static AuthorizedCardService authorizedCardService;

    // PIR 内存缓存
    private static volatile boolean lastPir = false;
    private static volatile String lastPirTime = "";

    // DHT 内存缓存
    private static volatile float lastTemp = 0;
    private static volatile float lastHumidity = 0;
    private static volatile String lastDhtTime = "";

    // MQTT 下发指令（由 MqttConfig 静态注入）
    private static String mqttBrokerUrl;

    public static void setBrokerUrl(String url) { mqttBrokerUrl = url; }

    // --- Getters for REST API ---
    public static boolean isPir()     { return lastPir; }
    public static String getPirTime() { return lastPirTime; }
    public static float getTemp()     { return lastTemp; }
    public static float getHumidity() { return lastHumidity; }
    public static String getDhtTime() { return lastDhtTime; }

    @Autowired public void setRfidService(RfidService s)             { rfidService = s; }
    @Autowired public void setAlarmService(AlarmService s)           { alarmService = s; }
    @Autowired public void setAuthorizedCardService(AuthorizedCardService s) { authorizedCardService = s; }

    @Override
    public void handleMessage(Message<?> message) throws MessagingException {
        String topic = (String) message.getHeaders().get("mqtt_receivedTopic");
        String payload = (String) message.getPayload();
        log.info("MQTT - Topic: {}, Payload: {}", topic, payload);
        if (topic == null || payload == null) return;
        try {
            switch (topic) {
                case "dorm/door/rfid":   handleRfid(payload);   break;
                case "dorm/door/pir":    handlePir(payload);    break;
                case "dorm/door/dht":    handleDht(payload);    break;
                case "dorm/door/alarm":  handleAlarm(payload);  break;
            }
        } catch (Exception e) {
            log.error("Error processing {}: {}", topic, e.getMessage());
        }
    }

    private void handleRfid(String payload) throws Exception {
        JsonNode json = objectMapper.readTree(payload);
        String uid = json.get("uid").asText();

        // 用后端数据库校验，不信任 ESP32 的 authorized 字段
        boolean dbAuth = authorizedCardService.isAuthorized(uid);
        String name = authorizedCardService.findByCardUid(uid)
                .map(c -> c.getHolderName())
                .orElse(null);

        RfidRecord r = new RfidRecord();
        r.setCardUid(uid);
        r.setIsAuthorized(dbAuth);
        r.setHolderName(name);
        r.setScanTime(LocalDateTime.now());
        rfidService.saveRecord(r);

        log.info("[RFID] uid={}, authorized={}, holder={}", uid, dbAuth, name);

        // ====== 下发开门指令给 ESP32 ======
        String cmdJson = objectMapper.writeValueAsString(new java.util.HashMap<String, Object>() {{
            put("cmd", dbAuth ? "open" : "deny");
            put("uid", uid);
            put("authorized", dbAuth);
        }});
        publishMqtt("dorm/door/cmd", cmdJson);

        // 未授权 → 自动生成告警
        if (!dbAuth) {
            AlarmLog a = new AlarmLog();
            a.setAlarmType("UNAUTHORIZED");
            a.setMessage("未授权刷卡: " + uid);
            a.setTriggeredAt(LocalDateTime.now());
            alarmService.save(a);
            log.warn("[ALARM] Unauthorized card: {}", uid);
        }
    }

    private void handlePir(String payload) throws Exception {
        JsonNode json = objectMapper.readTree(payload);
        lastPir = json.get("detected").asBoolean();
        lastPirTime = LocalDateTime.now().toString();
    }

    private void handleDht(String payload) throws Exception {
        JsonNode json = objectMapper.readTree(payload);
        if (json.has("temperature")) lastTemp = (float) json.get("temperature").asDouble();
        if (json.has("humidity")) lastHumidity = (float) json.get("humidity").asDouble();
        lastDhtTime = LocalDateTime.now().toString();
    }

    private void handleAlarm(String payload) throws Exception {
        JsonNode json = objectMapper.readTree(payload);
        AlarmLog a = new AlarmLog();
        a.setAlarmType(json.get("type").asText());
        a.setMessage(json.get("message").asText());
        a.setTriggeredAt(LocalDateTime.now());
        alarmService.save(a);
    }

    // ====== MQTT 下发指令 ======
    private static void publishMqtt(String topic, String payload) {
        try {
            MqttClient client = new MqttClient(mqttBrokerUrl,
                    "backend_cmd_" + System.currentTimeMillis());
            client.connect();
            MqttMessage msg = new MqttMessage(payload.getBytes());
            msg.setQos(1);
            client.publish(topic, msg);
            client.disconnect();
            client.close();
            log.info("[MQTT OUT] {} -> {}", topic, payload);
        } catch (Exception e) {
            log.error("[MQTT OUT] Failed to publish: {}", e.getMessage());
        }
    }
}
