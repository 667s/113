package com.dormsecurity.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpMethod;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.security.web.SecurityFilterChain;
import org.springframework.security.web.authentication.UsernamePasswordAuthenticationFilter;

@Configuration
@EnableWebSecurity
@EnableMethodSecurity(prePostEnabled = true)
public class SecurityConfig {

    private final JwtAuthFilter jwtAuthFilter;

    public SecurityConfig(JwtAuthFilter jwtAuthFilter) {
        this.jwtAuthFilter = jwtAuthFilter;
    }

    @Bean
    public BCryptPasswordEncoder passwordEncoder() {
        return new BCryptPasswordEncoder();
    }

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            .csrf(csrf -> csrf.disable())
            .cors(cors -> {})  // 使用已有的 CorsConfig
            .sessionManagement(session ->
                session.sessionCreationPolicy(SessionCreationPolicy.STATELESS))
            .authorizeHttpRequests(auth -> auth
                // 公开接口：登录、注册
                .requestMatchers("/api/auth/**").permitAll()
                // 公开接口：ESP32 查询卡授权 (MQTT 内部调用)
                .requestMatchers("/api/cards/check/**").permitAll()
                // 卡片管理：查看需登录，增删改仅管理员
                .requestMatchers(HttpMethod.GET, "/api/cards").authenticated()
                .requestMatchers(HttpMethod.POST, "/api/cards/**").hasRole("ADMIN")
                .requestMatchers(HttpMethod.PUT, "/api/cards/**").hasRole("ADMIN")
                .requestMatchers(HttpMethod.DELETE, "/api/cards/**").hasRole("ADMIN")
                // 其余 API：需要登录
                .requestMatchers("/api/**").authenticated()
                // 其他放行
                .anyRequest().permitAll()
            )
            .addFilterBefore(jwtAuthFilter, UsernamePasswordAuthenticationFilter.class);

        return http.build();
    }
}
