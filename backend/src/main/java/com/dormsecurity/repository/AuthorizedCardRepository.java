package com.dormsecurity.repository;

import com.dormsecurity.entity.AuthorizedCard;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface AuthorizedCardRepository extends JpaRepository<AuthorizedCard, Long> {

    /** 根据卡号查找授权卡 */
    Optional<AuthorizedCard> findByCardUid(String cardUid);

    /** 根据学号查找 */
    Optional<AuthorizedCard> findByStudentId(String studentId);

    /** 查找所有启用的卡 */
    List<AuthorizedCard> findByIsActiveTrue();

    /** 检查卡号是否存在 */
    boolean existsByCardUid(String cardUid);
}
