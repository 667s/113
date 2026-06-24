-- ============================================
-- 校园宿舍智能安防门 - 数据库初始化脚本
-- 数据库: dorm_security
-- ============================================

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS dorm_security
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE dorm_security;

-- 1. 授权卡表
CREATE TABLE IF NOT EXISTS authorized_cards (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    card_uid    VARCHAR(32)  NOT NULL UNIQUE COMMENT '卡片UID',
    holder_name VARCHAR(50)  NOT NULL COMMENT '持卡人姓名',
    student_id  VARCHAR(20)  DEFAULT NULL COMMENT '学号',
    is_active   TINYINT(1)   DEFAULT 1 COMMENT '是否启用 (1=启用, 0=停用)',
    created_at  DATETIME     DEFAULT CURRENT_TIMESTAMP COMMENT '添加时间',
    updated_at  DATETIME     DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='授权卡表';

-- 2. RFID刷卡记录表
CREATE TABLE IF NOT EXISTS rfid_records (
    id            BIGINT AUTO_INCREMENT PRIMARY KEY,
    card_uid      VARCHAR(32)  NOT NULL COMMENT '卡片UID',
    holder_name   VARCHAR(50)  DEFAULT NULL COMMENT '持卡人姓名（授权卡才有）',
    is_authorized TINYINT(1)   NOT NULL COMMENT '是否授权 (1=是, 0=否)',
    scan_time     DATETIME     DEFAULT CURRENT_TIMESTAMP COMMENT '刷卡时间',
    INDEX idx_scan_time (scan_time),
    INDEX idx_card_uid (card_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='RFID刷卡记录表';

-- 3. 传感器数据表（温湿度）
CREATE TABLE IF NOT EXISTS sensor_data (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT        DEFAULT NULL COMMENT '温度 (℃)',
    humidity    FLOAT        DEFAULT NULL COMMENT '湿度 (%)',
    recorded_at DATETIME     DEFAULT CURRENT_TIMESTAMP COMMENT '采集时间',
    INDEX idx_recorded_at (recorded_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='传感器数据表';

-- 4. 告警日志表
CREATE TABLE IF NOT EXISTS alarm_logs (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    alarm_type  VARCHAR(20)  NOT NULL COMMENT '告警类型: PIR/TEMP/HUMIDITY/UNAUTHORIZED',
    message     VARCHAR(255) NOT NULL COMMENT '告警消息内容',
    triggered_at DATETIME    DEFAULT CURRENT_TIMESTAMP COMMENT '触发时间',
    INDEX idx_triggered_at (triggered_at),
    INDEX idx_alarm_type (alarm_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='告警日志表';
