#pragma once

#include "../core/VisualEvent.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <graphics.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace dungeon {

enum class AnimationType {
    AttackSlash,
    MoveSlide,
    HitFlash,
    StunStars,
    PlayerFlash,
    DamageText,
    HealText,
    Explosion,
    SpikeTrigger,
    FireBurst,
    DeathBurst,
    WallDebris,
    HealParticles,
    ScreenShake,
    FloatingMessage
};

struct Animation {
    AnimationType type = AnimationType::AttackSlash;
    Position position{};
    Position targetPosition{};
    double startRow = 0.0;
    double startCol = 0.0;
    double targetRow = 0.0;
    double targetCol = 0.0;
    Direction direction = Direction::Right;
    MonsterType monsterType = MonsterType::Normal;
    DamageSource damageSource = DamageSource::PlayerAttack;
    int value = 0;
    int entityId = 0;
    bool playerEntity = false;
    bool drawGhost = false;
    std::int64_t startTimeMs = 0;
    std::int64_t durationMs = 1;
    COLORREF color = WHITE;
    std::wstring text;
};

struct GridPoint {
    double row = 0.0;
    double col = 0.0;
};

class AnimationManager {
public:
    void consumeEvents(const std::vector<VisualEvent>& events);
    void update();
    void clear() noexcept;

    [[nodiscard]] GridPoint playerDrawPosition(Position logicalPosition) const noexcept;
    [[nodiscard]] GridPoint monsterDrawPosition(int entityId, Position logicalPosition) const noexcept;
    [[nodiscard]] bool isMonsterFlashing(int entityId) const noexcept;
    [[nodiscard]] bool isPlayerFlashing() const noexcept;
    [[nodiscard]] bool isSpikeTriggered(Position position) const noexcept;
    [[nodiscard]] bool isPlayerDamageActive() const noexcept;
    [[nodiscard]] bool isPlayerHealActive() const noexcept;
    [[nodiscard]] std::pair<int, int> screenShakeOffset() const noexcept;

    void drawEffects(int worldOffsetX, int worldOffsetY) const;

private:
    static constexpr std::size_t MAX_ANIMATIONS = 512;

    std::vector<Animation> animations_;
    std::int64_t nowMs_ = 0;

    [[nodiscard]] static std::int64_t clockNowMs() noexcept;
    [[nodiscard]] double progress(const Animation& animation) const noexcept;
    [[nodiscard]] bool isActive(const Animation& animation) const noexcept;
    void add(Animation animation);
    void addMove(const VisualEvent& event, bool playerEntity, std::int64_t durationMs);
    void addScreenShake(int amplitude, std::int64_t durationMs, std::int64_t delayMs = 0);
    void addFloatingMessage(std::wstring text, COLORREF color, std::int64_t durationMs);
    [[nodiscard]] std::int64_t markDeathSlideAndGetDelay(int entityId);
};

} // namespace dungeon
