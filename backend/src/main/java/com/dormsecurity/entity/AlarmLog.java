package com.dormsecurity.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "alarm_logs")
public class AlarmLog {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "alarm_type", nullable = false, length = 20)
    private String alarmType;   // PIR / TEMP / HUMIDITY / UNAUTHORIZED

    @Column(name = "message", nullable = false, length = 255)
    private String message;

    @Column(name = "triggered_at")
    private LocalDateTime triggeredAt;

    @PrePersist
    protected void onCreate() {
        if (triggeredAt == null) {
            triggeredAt = LocalDateTime.now();
        }
    }

    // ===== Getters & Setters =====
    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getAlarmType() { return alarmType; }
    public void setAlarmType(String alarmType) { this.alarmType = alarmType; }

    public String getMessage() { return message; }
    public void setMessage(String message) { this.message = message; }

    public LocalDateTime getTriggeredAt() { return triggeredAt; }
    public void setTriggeredAt(LocalDateTime triggeredAt) { this.triggeredAt = triggeredAt; }
}
