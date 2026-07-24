#include "AnimationManager.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

namespace dungeon {
namespace {

COLORREF damageColor(DamageSource source) {
    switch (source) {
    case DamageSource::PlayerAttack:
    case DamageSource::MonsterAttack:
        return RGB(245, 80, 82);
    case DamageSource::Wall:
        return RGB(195, 205, 220);
    case DamageSource::Spike:
        return RGB(255, 65, 75);
    case DamageSource::Fire:
        return RGB(255, 132, 35);
    case DamageSource::BarrelExplosion:
    case DamageSource::BomberExplosion:
        return RGB(255, 210, 45);
    case DamageSource::Crush:
        return RGB(210, 115, 235);
    }
    return RGB(245, 80, 82);
}

COLORREF monsterColor(MonsterType type) {
    switch (type) {
    case MonsterType::Normal:
        return RGB(94, 218, 51);
    case MonsterType::Armor:
        return RGB(145, 82, 215);
    case MonsterType::Bomber:
        return RGB(245, 185, 35);
    case MonsterType::Boss:
        return RGB(165, 28, 38);
    }
    return RGB(180, 180, 180);
}

int cellCenterX(double col, int offsetX) {
    return static_cast<int>(std::lround(col * 40.0 + 20.0)) + offsetX;
}

int cellCenterY(double row, int offsetY) {
    return static_cast<int>(std::lround(row * 40.0 + 20.0)) + offsetY;
}

void useEffectFont(int height, COLORREF color, int weight = FW_BOLD) {
    settextstyle(height, 0, L"Microsoft YaHei UI", 0, 0, weight, false, false, false);
    settextcolor(color);
    setbkmode(TRANSPARENT);
}

} // namespace

std::int64_t AnimationManager::clockNowMs() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

double AnimationManager::progress(const Animation& animation) const noexcept {
    if (animation.durationMs <= 0) {
        return 1.0;
    }
    const double elapsed = static_cast<double>(nowMs_ - animation.startTimeMs);
    return std::clamp(elapsed / static_cast<double>(animation.durationMs), 0.0, 1.0);
}

bool AnimationManager::isActive(const Animation& animation) const noexcept {
    return nowMs_ >= animation.startTimeMs && nowMs_ - animation.startTimeMs < animation.durationMs;
}

void AnimationManager::add(Animation animation) {
    if (animation.durationMs <= 0) {
        animation.durationMs = 1;
    }
    if (animations_.size() >= MAX_ANIMATIONS) {
        animations_.erase(animations_.begin());
    }
    animations_.push_back(std::move(animation));
}

void AnimationManager::addMove(const VisualEvent& event, bool playerEntity, std::int64_t durationMs) {
    for (Animation& animation : animations_) {
        if (animation.type != AnimationType::MoveSlide || animation.playerEntity != playerEntity ||
            (!playerEntity && animation.entityId != event.entityId) || !isActive(animation)) {
            continue;
        }
        const double t = progress(animation);
        const double eased = 1.0 - (1.0 - t) * (1.0 - t);
        animation.startRow = animation.startRow + (animation.targetRow - animation.startRow) * eased;
        animation.startCol = animation.startCol + (animation.targetCol - animation.startCol) * eased;
        animation.targetRow = static_cast<double>(event.targetPosition.row);
        animation.targetCol = static_cast<double>(event.targetPosition.col);
        animation.targetPosition = event.targetPosition;
        animation.startTimeMs = nowMs_;
        animation.durationMs = durationMs;
        animation.direction = event.direction;
        return;
    }

    Animation animation;
    animation.type = AnimationType::MoveSlide;
    animation.position = event.position;
    animation.targetPosition = event.targetPosition;
    animation.startRow = static_cast<double>(event.position.row);
    animation.startCol = static_cast<double>(event.position.col);
    animation.targetRow = static_cast<double>(event.targetPosition.row);
    animation.targetCol = static_cast<double>(event.targetPosition.col);
    animation.direction = event.direction;
    animation.monsterType = event.monsterType;
    animation.entityId = event.entityId;
    animation.playerEntity = playerEntity;
    animation.startTimeMs = nowMs_;
    animation.durationMs = durationMs;
    add(std::move(animation));
}

void AnimationManager::addScreenShake(int amplitude, std::int64_t durationMs, std::int64_t delayMs) {
    Animation animation;
    animation.type = AnimationType::ScreenShake;
    animation.value = std::max(1, amplitude);
    animation.startTimeMs = nowMs_ + std::max<std::int64_t>(0, delayMs);
    animation.durationMs = durationMs;
    add(std::move(animation));
}

std::int64_t AnimationManager::markDeathSlideAndGetDelay(int entityId) {
    std::int64_t delay = 0;
    for (Animation& animation : animations_) {
        if (animation.type == AnimationType::MoveSlide && !animation.playerEntity &&
            animation.entityId == entityId && isActive(animation)) {
            animation.drawGhost = true;
            const std::int64_t remaining = animation.durationMs - (nowMs_ - animation.startTimeMs);
            delay = std::max(delay, std::max<std::int64_t>(0, remaining));
        }
    }
    return delay;
}

void AnimationManager::addFloatingMessage(std::wstring text, COLORREF color, std::int64_t durationMs) {
    if (text.empty()) {
        return;
    }
    Animation animation;
    animation.type = AnimationType::FloatingMessage;
    animation.text = std::move(text);
    animation.color = color;
    animation.startTimeMs = nowMs_;
    animation.durationMs = durationMs;
    add(std::move(animation));
}

void AnimationManager::consumeEvents(const std::vector<VisualEvent>& events) {
    nowMs_ = clockNowMs();
    for (const VisualEvent& event : events) {
        Animation animation;
        animation.position = event.position;
        animation.targetPosition = event.targetPosition;
        animation.direction = event.direction;
        animation.monsterType = event.monsterType;
        animation.damageSource = event.damageSource;
        animation.value = event.value;
        animation.entityId = event.entityId;
        animation.startTimeMs = nowMs_;

        switch (event.type) {
        case VisualEventType::PlayerAttack:
            animation.type = AnimationType::AttackSlash;
            animation.durationMs = 200;
            animation.color = RGB(255, 221, 105);
            add(std::move(animation));
            break;
        case VisualEventType::PlayerMoved:
            addMove(event, true, 105);
            break;
        case VisualEventType::MonsterMoved:
            addMove(event, false, 110);
            break;
        case VisualEventType::MonsterPushed:
            addMove(event, false, 135);
            break;
        case VisualEventType::MonsterHit: {
            animation.type = AnimationType::HitFlash;
            animation.durationMs = 100;
            add(animation);
            animation.type = AnimationType::DamageText;
            animation.durationMs = 720;
            animation.color = damageColor(event.damageSource);
            add(std::move(animation));
            break;
        }
        case VisualEventType::MonsterStunned:
            animation.type = AnimationType::StunStars;
            animation.durationMs = 620;
            animation.color = RGB(255, 221, 82);
            add(std::move(animation));
            break;
        case VisualEventType::WallHit:
            animation.type = AnimationType::WallDebris;
            animation.durationMs = 240;
            animation.color = RGB(190, 198, 210);
            add(std::move(animation));
            addScreenShake(2, 85);
            break;
        case VisualEventType::SpikeHit:
            animation.type = AnimationType::SpikeTrigger;
            animation.durationMs = 180;
            animation.color = RGB(255, 55, 65);
            add(std::move(animation));
            break;
        case VisualEventType::FireDamage:
            animation.type = AnimationType::FireBurst;
            animation.durationMs = 280;
            animation.color = RGB(255, 125, 25);
            add(std::move(animation));
            break;
        case VisualEventType::BarrelExplode:
            animation.type = AnimationType::Explosion;
            animation.durationMs = 350;
            animation.color = RGB(255, 195, 35);
            add(std::move(animation));
            addScreenShake(4, 120);
            break;
        case VisualEventType::BomberExplode: {
            const std::int64_t delay = event.entityId > 0 ? markDeathSlideAndGetDelay(event.entityId) : 0;
            animation.type = AnimationType::Explosion;
            animation.durationMs = 285;
            animation.color = RGB(255, 145, 30);
            animation.startTimeMs += delay;
            add(std::move(animation));
            addScreenShake(3, 95, delay);
            break;
        }
        case VisualEventType::MonsterDeath: {
            const std::int64_t delay = event.entityId > 0 ? markDeathSlideAndGetDelay(event.entityId) : 0;
            if (event.monsterType != MonsterType::Bomber) {
                animation.type = AnimationType::DeathBurst;
                animation.durationMs = event.monsterType == MonsterType::Boss ? 520 : 300;
                animation.color = monsterColor(event.monsterType);
                animation.startTimeMs += delay;
                add(std::move(animation));
                if (event.monsterType == MonsterType::Boss) {
                    addScreenShake(5, 300, delay);
                }
            }
            break;
        }
        case VisualEventType::PlayerDamaged: {
            animation.type = AnimationType::PlayerFlash;
            animation.playerEntity = true;
            animation.durationMs = 130;
            add(animation);
            animation.type = AnimationType::DamageText;
            animation.durationMs = 720;
            animation.color = damageColor(event.damageSource);
            add(std::move(animation));
            addScreenShake(event.value >= 4 ? 3 : 1, event.value >= 4 ? 110 : 70);
            break;
        }
        case VisualEventType::PlayerHealed: {
            animation.type = AnimationType::HealText;
            animation.playerEntity = true;
            animation.durationMs = 760;
            animation.color = RGB(80, 235, 125);
            add(animation);
            animation.type = AnimationType::HealParticles;
            animation.durationMs = 560;
            add(std::move(animation));
            break;
        }
        case VisualEventType::ExitOpened:
            addFloatingMessage(event.message.empty() ? L"出口开启！" : event.message,
                               RGB(255, 214, 80), 1050);
            break;
        case VisualEventType::BossRage:
            addFloatingMessage(event.message.empty() ? L"BOSS 狂暴！" : event.message,
                               RGB(255, 70, 75), 1200);
            addScreenShake(5, 190);
            break;
        case VisualEventType::LevelClear:
            addFloatingMessage(event.message.empty() ? L"关卡通过！" : event.message,
                               RGB(255, 216, 88), 1000);
            break;
        case VisualEventType::GameOver:
            addFloatingMessage(event.message.empty() ? L"你倒下了……" : event.message,
                               RGB(245, 80, 85), 1100);
            break;
        case VisualEventType::GameWin:
            addFloatingMessage(event.message.empty() ? L"通关成功！" : event.message,
                               RGB(255, 222, 95), 1400);
            break;
        case VisualEventType::UpgradeApplied:
            addFloatingMessage(event.message.empty() ? L"能力提升！" : event.message,
                               RGB(105, 195, 255), 1050);
            break;
        }
    }
}

void AnimationManager::update() {
    nowMs_ = clockNowMs();
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(), [this](const Animation& animation) {
            return nowMs_ - animation.startTimeMs >= animation.durationMs;
        }),
        animations_.end());
}

void AnimationManager::clear() noexcept {
    animations_.clear();
}

GridPoint AnimationManager::playerDrawPosition(Position logicalPosition) const noexcept {
    for (auto it = animations_.rbegin(); it != animations_.rend(); ++it) {
        if (it->type == AnimationType::MoveSlide && it->playerEntity && isActive(*it)) {
            const double t = progress(*it);
            const double eased = 1.0 - (1.0 - t) * (1.0 - t);
            return {it->startRow + (it->targetRow - it->startRow) * eased,
                    it->startCol + (it->targetCol - it->startCol) * eased};
        }
    }
    return {static_cast<double>(logicalPosition.row), static_cast<double>(logicalPosition.col)};
}

GridPoint AnimationManager::monsterDrawPosition(int entityId, Position logicalPosition) const noexcept {
    for (auto it = animations_.rbegin(); it != animations_.rend(); ++it) {
        if (it->type == AnimationType::MoveSlide && !it->playerEntity && it->entityId == entityId &&
            isActive(*it)) {
            const double t = progress(*it);
            const double eased = 1.0 - (1.0 - t) * (1.0 - t);
            return {it->startRow + (it->targetRow - it->startRow) * eased,
                    it->startCol + (it->targetCol - it->startCol) * eased};
        }
    }
    return {static_cast<double>(logicalPosition.row), static_cast<double>(logicalPosition.col)};
}

bool AnimationManager::isMonsterFlashing(int entityId) const noexcept {
    return std::any_of(animations_.begin(), animations_.end(), [this, entityId](const Animation& animation) {
        return animation.type == AnimationType::HitFlash && animation.entityId == entityId && isActive(animation);
    });
}

bool AnimationManager::isPlayerFlashing() const noexcept {
    return std::any_of(animations_.begin(), animations_.end(), [this](const Animation& animation) {
        return animation.type == AnimationType::PlayerFlash && isActive(animation);
    });
}

bool AnimationManager::isSpikeTriggered(Position position) const noexcept {
    return std::any_of(animations_.begin(), animations_.end(), [this, position](const Animation& animation) {
        return animation.type == AnimationType::SpikeTrigger && animation.position == position && isActive(animation);
    });
}

bool AnimationManager::isPlayerDamageActive() const noexcept {
    return isPlayerFlashing();
}

bool AnimationManager::isPlayerHealActive() const noexcept {
    return std::any_of(animations_.begin(), animations_.end(), [this](const Animation& animation) {
        return animation.type == AnimationType::HealText && animation.playerEntity && isActive(animation);
    });
}

std::pair<int, int> AnimationManager::screenShakeOffset() const noexcept {
    int amplitude = 0;
    for (const Animation& animation : animations_) {
        if (animation.type == AnimationType::ScreenShake && isActive(animation)) {
            amplitude = std::max(amplitude, animation.value);
        }
    }
    if (amplitude <= 0) {
        return {0, 0};
    }
    const int phase = static_cast<int>((nowMs_ / 18) % 4);
    const std::array<std::pair<int, int>, 4> directions{{
        {amplitude, 0}, {-amplitude, amplitude / 2}, {0, -amplitude}, {amplitude / 2, amplitude}
    }};
    return directions[static_cast<std::size_t>(phase)];
}

void AnimationManager::drawEffects(int worldOffsetX, int worldOffsetY) const {
    for (const Animation& animation : animations_) {
        if (!isActive(animation)) {
            continue;
        }
        const double t = progress(animation);
        const int centerX = cellCenterX(static_cast<double>(animation.position.col), worldOffsetX);
        const int centerY = cellCenterY(static_cast<double>(animation.position.row), worldOffsetY);
        const int targetX = cellCenterX(static_cast<double>(animation.targetPosition.col), worldOffsetX);
        const int targetY = cellCenterY(static_cast<double>(animation.targetPosition.row), worldOffsetY);

        switch (animation.type) {
        case AnimationType::AttackSlash: {
            const Position offset = directionOffset(animation.direction);
            const int impactX = targetX;
            const int impactY = targetY;
            const Position side{-offset.col, offset.row};
            const int reveal = std::clamp(static_cast<int>(std::lround(t * 9.0)), 1, 7);
            for (int index = 0; index < reveal; ++index) {
                const int sideDistance = -15 + index * 5;
                const int curve = 5 + (15 - std::abs(sideDistance)) / 3;
                // 月牙弧心必须位于目标格的攻击方向外侧；使用正向偏移可保证
                // 上/下/左/右四种朝向都从玩家向目标展开，而不是反向卷回玩家。
                const int x = impactX + offset.col * curve + side.col * sideDistance;
                const int y = impactY + offset.row * curve + side.row * sideDistance;
                setfillcolor(t < 0.72 ? RGB(33, 105, 238) : RGB(35, 67, 132));
                solidrectangle(x - 4, y - 4, x + 4, y + 4);
                setfillcolor(t < 0.72 ? RGB(139, 230, 255) : RGB(86, 144, 189));
                solidrectangle(x - 2, y - 3, x + 2, y + 3);
                setfillcolor(WHITE);
                solidrectangle(x - 1, y - 2, x + 1, y + 2);
            }
            setfillcolor(t < 0.58 ? WHITE : RGB(119, 187, 221));
            solidrectangle(impactX - 2, impactY - 13, impactX + 2, impactY - 8);
            solidrectangle(impactX - 2, impactY + 8, impactX + 2, impactY + 13);
            solidrectangle(impactX - 13, impactY - 2, impactX - 8, impactY + 2);
            solidrectangle(impactX + 8, impactY - 2, impactX + 13, impactY + 2);
            break;
        }
        case AnimationType::DamageText:
        case AnimationType::HealText: {
            std::wstring text;
            if (animation.type == AnimationType::HealText) {
                text = animation.value > 0 ? L"+" + std::to_wstring(animation.value) : L"HP 已满";
            } else {
                text = L"-" + std::to_wstring(animation.value);
            }
            const int y = centerY - 17 - static_cast<int>(std::lround(24.0 * t));
            useEffectFont(animation.monsterType == MonsterType::Boss && !animation.playerEntity ? 21 : 17,
                          animation.color);
            outtextxy(centerX - textwidth(text.c_str()) / 2, y, text.c_str());
            break;
        }
        case AnimationType::Explosion: {
            const double wave = std::min(1.0, t / 0.62);
            const int radius = 5 + static_cast<int>(std::lround(24.0 * wave));
            const COLORREF color = t < 0.32 ? RGB(255, 245, 165)
                                           : (t < 0.72 ? animation.color : RGB(120, 96, 82));
            setfillcolor(color);
            solidrectangle(centerX - radius, centerY - radius, centerX + radius, centerY + radius);
            setfillcolor(t < 0.45 ? RGB(255, 250, 190) : RGB(176, 83, 42));
            solidrectangle(centerX - radius / 2, centerY - radius / 2,
                           centerX + radius / 2, centerY + radius / 2);
            if (t > 0.18) {
                const int reach = static_cast<int>(std::lround(55.0 * std::min(1.0, (t - 0.18) / 0.45)));
                setfillcolor(t < 0.72 ? RGB(255, 150, 35) : RGB(105, 88, 78));
                solidrectangle(centerX - 7, centerY - reach, centerX + 7, centerY + reach);
                solidrectangle(centerX - reach, centerY - 7, centerX + reach, centerY + 7);
                setfillcolor(color);
                const int core = std::max(4, radius - 5);
                solidrectangle(centerX - core, centerY - core, centerX + core, centerY + core);
            }
            break;
        }
        case AnimationType::SpikeTrigger: {
            const int lift = t < 0.5 ? 3 : 1;
            setlinecolor(RGB(255, 65, 70));
            setlinestyle(PS_SOLID, 3);
            rectangle(centerX - 18, centerY - 18 - lift, centerX + 18, centerY + 18);
            setlinestyle(PS_SOLID, 1);
            break;
        }
        case AnimationType::FireBurst: {
            const std::array<int, 5> xOffsets{{-12, -6, 0, 7, 13}};
            for (std::size_t i = 0; i < xOffsets.size(); ++i) {
                const int rise = static_cast<int>(std::lround((12.0 + static_cast<double>(i % 3) * 5.0) * t));
                setfillcolor(i % 2 == 0 ? RGB(255, 185, 35) : RGB(245, 78, 25));
                const int size = std::max(1, 4 - static_cast<int>(t * 3.0));
                solidrectangle(centerX + xOffsets[i] - size, centerY + 10 - rise - size,
                               centerX + xOffsets[i] + size, centerY + 10 - rise + size);
            }
            break;
        }
        case AnimationType::DeathBurst: {
            const int bodyRadius = std::max(1, static_cast<int>(std::lround(15.0 * (1.0 - t))));
            setfillcolor(animation.color);
            solidrectangle(centerX - bodyRadius, centerY - bodyRadius,
                           centerX + bodyRadius, centerY + bodyRadius);
            const std::array<Position, 8> directions{{
                {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
            }};
            const int distance = static_cast<int>(std::lround(24.0 * t));
            const int size = std::max(1, static_cast<int>(std::lround(4.0 * (1.0 - t))));
            for (const Position direction : directions) {
                setfillcolor(animation.monsterType == MonsterType::Boss && (direction.row + direction.col) % 2 == 0
                                 ? RGB(245, 195, 55)
                                 : animation.color);
                solidrectangle(centerX + direction.col * distance - size,
                               centerY + direction.row * distance - size,
                               centerX + direction.col * distance + size,
                               centerY + direction.row * distance + size);
            }
            break;
        }
        case AnimationType::WallDebris: {
            const Position offset = directionOffset(animation.direction);
            const std::array<int, 5> spread{{-10, -5, 0, 6, 11}};
            for (std::size_t i = 0; i < spread.size(); ++i) {
                const int distance = 8 + static_cast<int>(std::lround(18.0 * t));
                const int x = centerX + offset.col * distance + (offset.row != 0 ? spread[i] : 0);
                const int y = centerY + offset.row * distance + (offset.col != 0 ? spread[i] : 0);
                setfillcolor(i % 2 == 0 ? RGB(185, 195, 208) : RGB(110, 120, 136));
                solidrectangle(x - 2, y - 2, x + 2, y + 2);
            }
            break;
        }
        case AnimationType::HealParticles: {
            const std::array<int, 6> xOffsets{{-13, -8, -2, 4, 9, 14}};
            for (std::size_t i = 0; i < xOffsets.size(); ++i) {
                const int rise = static_cast<int>(std::lround((18.0 + static_cast<double>(i % 2) * 8.0) * t));
                setfillcolor(i % 2 == 0 ? RGB(90, 235, 130) : RGB(185, 255, 205));
                solidrectangle(centerX + xOffsets[i] - 2, centerY + 10 - rise,
                               centerX + xOffsets[i] + 2, centerY + 14 - rise);
            }
            break;
        }
        case AnimationType::FloatingMessage: {
            useEffectFont(27, animation.color);
            const int panelWidth = std::max(250, textwidth(animation.text.c_str()) + 70);
            const int left = 400 - panelWidth / 2;
            const int top = 55 + static_cast<int>(std::lround(8.0 * (1.0 - t)));
            setfillcolor(RGB(11, 14, 20));
            solidrectangle(left + 4, top + 4, left + panelWidth + 4, top + 56);
            setfillcolor(animation.color);
            solidrectangle(left, top, left + panelWidth, top + 52);
            setfillcolor(RGB(25, 31, 43));
            solidrectangle(left + 3, top + 3, left + panelWidth - 3, top + 49);
            outtextxy(400 - textwidth(animation.text.c_str()) / 2, top + 10, animation.text.c_str());
            break;
        }
        case AnimationType::MoveSlide:
            if (animation.drawGhost) {
                const double eased = 1.0 - (1.0 - t) * (1.0 - t);
                const double drawRow = animation.startRow + (animation.targetRow - animation.startRow) * eased;
                const double drawCol = animation.startCol + (animation.targetCol - animation.startCol) * eased;
                const int ghostX = cellCenterX(drawCol, worldOffsetX);
                const int ghostY = cellCenterY(drawRow, worldOffsetY);
                const int size = animation.monsterType == MonsterType::Boss ? 18 : 14;
                setfillcolor(RGB(20, 21, 27));
                solidrectangle(ghostX - size, ghostY - size, ghostX + size, ghostY + size);
                setfillcolor(isMonsterFlashing(animation.entityId) ? WHITE : monsterColor(animation.monsterType));
                solidrectangle(ghostX - size + 3, ghostY - size + 3,
                               ghostX + size - 3, ghostY + size - 3);
            }
            break;
        case AnimationType::StunStars: {
            const double angle = t * 6.283185307179586;
            const std::array<double, 3> offsets{{0.0, 2.0943951023931953, 4.1887902047863905}};
            for (double offset : offsets) {
                const int starX = centerX + static_cast<int>(std::lround(std::cos(angle + offset) * 15.0));
                const int starY = centerY - 22 + static_cast<int>(std::lround(std::sin(angle + offset) * 4.0));
                setfillcolor(RGB(25, 22, 25));
                solidrectangle(starX - 3, starY - 3, starX + 3, starY + 3);
                setfillcolor(animation.color);
                solidrectangle(starX - 1, starY - 5, starX + 1, starY + 5);
                solidrectangle(starX - 5, starY - 1, starX + 5, starY + 1);
            }
            break;
        }
        case AnimationType::HitFlash:
        case AnimationType::PlayerFlash:
        case AnimationType::ScreenShake:
            break;
        }
    }
}

} // namespace dungeon
