#pragma once

#include "Types.h"

#include <string>

namespace dungeon {

// 后端只发布已经发生的事实；UI 可以用它播放动画，但不能反向改变规则。
enum class VisualEventType {
    PlayerAttack,
    PlayerMoved,
    MonsterMoved,
    MonsterHit,
    MonsterStunned,
    MonsterPushed,
    WallHit,
    SpikeHit,
    FireDamage,
    BarrelExplode,
    BomberExplode,
    MonsterDeath,
    PlayerDamaged,
    PlayerHealed,
    ExitOpened,
    BossRage,
    LevelClear,
    GameOver,
    GameWin,
    UpgradeApplied
};

struct VisualEvent {
    VisualEventType type = VisualEventType::PlayerAttack;
    Position position{};
    Position targetPosition{};
    int value = 0;
    int entityId = 0;
    Direction direction = Direction::Right;
    MonsterType monsterType = MonsterType::Normal;
    DamageSource damageSource = DamageSource::PlayerAttack;
    std::wstring message;
};

enum class LogCategory {
    System,
    PlayerAction,
    Damage,
    Fire,
    Explosion,
    Heal,
    Success,
    Danger
};

struct LogEntry {
    std::wstring text;
    LogCategory category = LogCategory::System;
};

} // namespace dungeon
