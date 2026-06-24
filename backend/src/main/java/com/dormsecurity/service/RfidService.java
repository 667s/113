package com.dormsecurity.service;

import com.dormsecurity.entity.RfidRecord;
import com.dormsecurity.repository.RfidRecordRepository;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class RfidService {

    private final RfidRecordRepository rfidRecordRepository;

    public RfidService(RfidRecordRepository rfidRecordRepository) {
        this.rfidRecordRepository = rfidRecordRepository;
    }

    /** 保存刷卡记录 */
    public RfidRecord saveRecord(RfidRecord record) {
        return rfidRecordRepository.save(record);
    }

    /** 分页查询所有记录（按时间降序） */
    public Page<RfidRecord> findAll(Pageable pageable) {
        return rfidRecordRepository.findAllByOrderByScanTimeDesc(pageable);
    }

    /** 获取最新10条记录 */
    public List<RfidRecord> findLatest() {
        return rfidRecordRepository.findTop10ByOrderByScanTimeDesc();
    }

    /** 按时间范围查询 */
    public List<RfidRecord> findByTimeRange(LocalDateTime start, LocalDateTime end) {
        return rfidRecordRepository.findByScanTimeBetweenOrderByScanTimeDesc(start, end);
    }

    /** 按授权状态分页查询 */
    public Page<RfidRecord> findByAuthorized(Boolean isAuthorized, Pageable pageable) {
        return rfidRecordRepository.findByIsAuthorizedOrderByScanTimeDesc(isAuthorized, pageable);
    }
}
