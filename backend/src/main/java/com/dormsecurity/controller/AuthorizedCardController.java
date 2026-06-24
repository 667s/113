package com.dormsecurity.controller;

import com.dormsecurity.entity.AuthorizedCard;
import com.dormsecurity.service.AuthorizedCardService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/cards")
public class AuthorizedCardController {

    private final AuthorizedCardService cardService;

    public AuthorizedCardController(AuthorizedCardService cardService) {
        this.cardService = cardService;
    }

    /**
     * ESP32 刷卡时查询卡是否授权 (轻量接口，只返回授权状态)
     * GET /api/cards/check/{cardUid}
     * @return {"authorized": true/false, "holder_name": "..."}
     */
    @GetMapping("/check/{cardUid}")
    public ResponseEntity<Map<String, Object>> checkCard(@PathVariable String cardUid) {
        Map<String, Object> result = new HashMap<>();
        result.put("authorized", cardService.isAuthorized(cardUid));
        result.put("holder_name", cardService.findByCardUid(cardUid)
                .map(AuthorizedCard::getHolderName).orElse(""));
        return ResponseEntity.ok(result);
    }

    /**
     * 查询所有授权卡
     * GET /api/cards
     */
    @GetMapping
    public ResponseEntity<List<AuthorizedCard>> getAll() {
        return ResponseEntity.ok(cardService.findAll());
    }

    /**
     * 添加授权卡
     * POST /api/cards
     * Body: { "cardUid": "...", "holderName": "...", "studentId": "..." }
     */
    @PostMapping
    public ResponseEntity<?> addCard(@RequestBody AuthorizedCard card) {
        try {
            AuthorizedCard saved = cardService.addCard(card);
            return ResponseEntity.ok(saved);
        } catch (RuntimeException e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }

    /**
     * 删除授权卡
     * DELETE /api/cards/{id}
     */
    @DeleteMapping("/{id}")
    public ResponseEntity<?> deleteCard(@PathVariable Long id) {
        try {
            cardService.deleteCard(id);
            return ResponseEntity.ok(Map.of("message", "删除成功"));
        } catch (RuntimeException e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }

    /**
     * 更新卡状态（启用/停用）
     * PUT /api/cards/{id}/status
     * Body: { "isActive": true/false }
     */
    @PutMapping("/{id}/status")
    public ResponseEntity<?> updateStatus(
            @PathVariable Long id,
            @RequestBody Map<String, Boolean> body) {
        try {
            Boolean isActive = body.get("isActive");
            AuthorizedCard updated = cardService.updateStatus(id, isActive);
            return ResponseEntity.ok(updated);
        } catch (RuntimeException e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }
}
