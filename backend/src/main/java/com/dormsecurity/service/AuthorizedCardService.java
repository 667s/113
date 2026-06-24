package com.dormsecurity.service;

import com.dormsecurity.entity.AuthorizedCard;
import com.dormsecurity.repository.AuthorizedCardRepository;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Optional;

@Service
public class AuthorizedCardService {

    private final AuthorizedCardRepository authorizedCardRepository;

    public AuthorizedCardService(AuthorizedCardRepository authorizedCardRepository) {
        this.authorizedCardRepository = authorizedCardRepository;
    }

    /** 查询所有授权卡 */
    public List<AuthorizedCard> findAll() {
        return authorizedCardRepository.findAll();
    }

    /** 按卡号查找 */
    public Optional<AuthorizedCard> findByCardUid(String cardUid) {
        return authorizedCardRepository.findByCardUid(cardUid);
    }

    /** 检查卡号是否已授权 */
    public boolean isAuthorized(String cardUid) {
        return authorizedCardRepository.findByCardUid(cardUid)
                .map(AuthorizedCard::getIsActive)
                .orElse(false);
    }

    /** 添加授权卡 */
    public AuthorizedCard addCard(AuthorizedCard card) {
        if (authorizedCardRepository.existsByCardUid(card.getCardUid())) {
            throw new RuntimeException("该卡号已存在: " + card.getCardUid());
        }
        return authorizedCardRepository.save(card);
    }

    /** 删除授权卡 */
    public void deleteCard(Long id) {
        authorizedCardRepository.deleteById(id);
    }

    /** 更新授权卡状态 */
    public AuthorizedCard updateStatus(Long id, Boolean isActive) {
        AuthorizedCard card = authorizedCardRepository.findById(id)
                .orElseThrow(() -> new RuntimeException("卡不存在: " + id));
        card.setIsActive(isActive);
        return authorizedCardRepository.save(card);
    }
}
