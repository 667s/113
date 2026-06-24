package com.dormsecurity;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class DormSecurityApplication {

    public static void main(String[] args) {
        SpringApplication.run(DormSecurityApplication.class, args);
        System.out.println("===========================================");
        System.out.println("  校园宿舍智能安防门 - 后端服务启动成功!");
        System.out.println("  http://localhost:8080");
        System.out.println("===========================================");
    }
}
