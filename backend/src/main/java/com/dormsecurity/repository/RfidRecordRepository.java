package com.dormsecurity.repository;

import com.dormsecurity.entity.RfidRecord;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;

@Repository
public interface RfidRecordRepository extends JpaRepository<RfidRecord, Long> {

    /** 按时间降序分页查询所有记录 */
    Page<RfidRecord> findAllByOrderByScanTimeDesc(Pageable pageable);

    /** 查询指定时间范围内的记录 */
    List<RfidRecord> findByScanTimeBetweenOrderByScanTimeDesc(
            LocalDateTime start, LocalDateTime end);

    /** 查询最新的N条记录 */
    List<RfidRecord> findTop10ByOrderByScanTimeDesc();

    /** 按授权状态查询 */
    Page<RfidRecord> findByIsAuthorizedOrderByScanTimeDesc(
            Boolean isAuthorized, Pageable pageable);
}
