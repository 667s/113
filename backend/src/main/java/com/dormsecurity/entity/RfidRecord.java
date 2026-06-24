package com.dormsecurity.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "rfid_records")
public class RfidRecord {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "card_uid", nullable = false, length = 32)
    private String cardUid;

    @Column(name = "holder_name", length = 50)
    private String holderName;

    @Column(name = "is_authorized", nullable = false)
    private Boolean isAuthorized;

    @Column(name = "scan_time")
    private LocalDateTime scanTime;

    @PrePersist
    protected void onCreate() {
        if (scanTime == null) {
            scanTime = LocalDateTime.now();
        }
    }

    // ===== Getters & Setters =====
    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getCardUid() { return cardUid; }
    public void setCardUid(String cardUid) { this.cardUid = cardUid; }

    public String getHolderName() { return holderName; }
    public void setHolderName(String holderName) { this.holderName = holderName; }

    public Boolean getIsAuthorized() { return isAuthorized; }
    public void setIsAuthorized(Boolean isAuthorized) { this.isAuthorized = isAuthorized; }

    public LocalDateTime getScanTime() { return scanTime; }
    public void setScanTime(LocalDateTime scanTime) { this.scanTime = scanTime; }
}
