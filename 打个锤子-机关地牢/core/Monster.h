#pragma once

#include "Types.h"

#include <string_view>

namespace dungeon {

class Monster {
public:
    Monster(MonsterType monsterType, Position spawnPosition, int entityId = 0);

    Position position;
    int id = 0;
    int hp = 0;
    int maxHp = 0;
    int attack = 0;
    int moveInterval = 1;
    int lastMoveTurn = 0;
    int stunTurnsRemaining = 0;
    MonsterType type = MonsterType::Normal;
    bool alive = true;

    [[nodiscard]] bool isAlive() const noexcept;
    void takeDamage(int damage) noexcept;
    void stunForOneTurn() noexcept;
    [[nodiscard]] bool isStunned() const noexcept;
    [[nodiscard]] bool consumeStunTurn() noexcept;
    [[nodiscard]] int attackDamage() const noexcept;
    [[nodiscard]] wchar_t symbol() const noexcept;
    [[nodiscard]] std::wstring_view displayName() const noexcept;
};

} // namespace dungeon
