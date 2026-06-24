package com.dormsecurity.config;

import com.dormsecurity.handler.MqttInboundHandler;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.integration.channel.DirectChannel;
import org.springframework.integration.core.MessageProducer;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.outbound.MqttPahoMessageHandler;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.MessageHandler;

import java.util.List;

@Configuration
public class MqttConfig {

    @Value("${mqtt.broker-url}")
    private String brokerUrl;

    @Value("${mqtt.client-id}")
    private String clientId;

    @Value("${mqtt.username:}")
    private String username;

    @Value("${mqtt.password:}")
    private String password;

    @Value("${mqtt.qos:1}")
    private int qos;

    @Value("${mqtt.completion-timeout:5000}")
    private int completionTimeout;

    @Value("${mqtt.topics}")
    private List<String> topics;

    /**
     * MQTT 客户端工厂
     */
    @Bean
    public MqttPahoClientFactory mqttClientFactory() {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{brokerUrl});
        options.setCleanSession(true);
        options.setAutomaticReconnect(true);
        options.setConnectionTimeout(10);
        options.setKeepAliveInterval(60);

        if (username != null && !username.isEmpty()) {
            options.setUserName(username);
        }
        if (password != null && !password.isEmpty()) {
            options.setPassword(password.toCharArray());
        }

        factory.setConnectionOptions(options);
        return factory;
    }

    /**
     * MQTT 入站消息通道
     */
    @Bean
    public MessageChannel mqttInputChannel() {
        return new DirectChannel();
    }

    /**
     * MQTT 入站适配器（订阅所有主题）
     */
    @Bean
    public MessageProducer mqttInboundAdapter(MqttPahoClientFactory mqttClientFactory) {
        String inboundClientId = clientId + "_inbound_" + System.currentTimeMillis();
        MqttPahoMessageDrivenChannelAdapter adapter =
                new MqttPahoMessageDrivenChannelAdapter(inboundClientId, mqttClientFactory);

        // 订阅所有配置的主题
        for (String topic : topics) {
            adapter.addTopic(topic, qos);
        }

        adapter.setCompletionTimeout(completionTimeout);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setQos(qos);
        adapter.setOutputChannel(mqttInputChannel());

        return adapter;
    }

    /**
     * MQTT 入站消息处理器
     */
    @Bean
    @ServiceActivator(inputChannel = "mqttInputChannel")
    public MessageHandler mqttInboundHandler() {
        MqttInboundHandler handler = new MqttInboundHandler();
        MqttInboundHandler.setBrokerUrl(brokerUrl);
        return handler;
    }

    /**
     * MQTT 出站通道
     */
    @Bean
    public MessageChannel mqttOutputChannel() {
        return new DirectChannel();
    }

    /**
     * MQTT 出站适配器（用于向ESP32/前端发送消息）
     */
    @Bean
    @ServiceActivator(inputChannel = "mqttOutputChannel")
    public MessageHandler mqttOutboundAdapter(MqttPahoClientFactory mqttClientFactory) {
        String outboundClientId = clientId + "_outbound_" + System.currentTimeMillis();
        MqttPahoMessageHandler handler = new MqttPahoMessageHandler(outboundClientId, mqttClientFactory);
        handler.setAsync(true);
        handler.setDefaultQos(qos);
        return handler;
    }
}
