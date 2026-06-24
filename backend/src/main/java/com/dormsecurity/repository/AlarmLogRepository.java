package com.dormsecurity.repository;

import com.dormsecurity.entity.AlarmLog;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;

@Repository
public interface AlarmLogRepository extends JpaRepository<AlarmLog, Long> {

    /** 按时间降序分页查询所有告警 */
    Page<AlarmLog> findAllByOrderByTriggeredAtDesc(Pageable pageable);

    /** 按告警类型分页查询 */
    Page<AlarmLog> findByAlarmTypeOrderByTriggeredAtDesc(
            String alarmType, Pageable pageable);

    /** 查询最新的N条告警 */
    List<AlarmLog> findTop10ByOrderByTriggeredAtDesc();

    /** 查询指定时间范围内的告警 */
    List<AlarmLog> findByTriggeredAtBetweenOrderByTriggeredAtDesc(
            LocalDateTime start, LocalDateTime end);
}
