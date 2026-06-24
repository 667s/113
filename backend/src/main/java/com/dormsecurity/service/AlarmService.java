package com.dormsecurity.service;

import com.dormsecurity.entity.AlarmLog;
import com.dormsecurity.repository.AlarmLogRepository;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class AlarmService {

    private final AlarmLogRepository alarmLogRepository;

    public AlarmService(AlarmLogRepository alarmLogRepository) {
        this.alarmLogRepository = alarmLogRepository;
    }

    /** 保存告警记录 */
    public AlarmLog save(AlarmLog log) {
        return alarmLogRepository.save(log);
    }

    /** 分页查询所有告警 */
    public Page<AlarmLog> findAll(Pageable pageable) {
        return alarmLogRepository.findAllByOrderByTriggeredAtDesc(pageable);
    }

    /** 按类型分页查询告警 */
    public Page<AlarmLog> findByType(String alarmType, Pageable pageable) {
        return alarmLogRepository.findByAlarmTypeOrderByTriggeredAtDesc(alarmType, pageable);
    }

    /** 获取最新10条告警 */
    public List<AlarmLog> findLatest() {
        return alarmLogRepository.findTop10ByOrderByTriggeredAtDesc();
    }

    /** 按时间范围查询 */
    public List<AlarmLog> findByTimeRange(LocalDateTime start, LocalDateTime end) {
        return alarmLogRepository.findByTriggeredAtBetweenOrderByTriggeredAtDesc(start, end);
    }
}
