#pragma once

#include "Types.h"

namespace dungeon {

class Player {
public:
    Position position{1, 1};
    int hp = 20;
    int maxHp = 20;
    int attack = 1;
    int pushPower = 1;
    Direction direction = Direction::Right;

    int killCount = 0;
    int trapKillCount = 0;
    int turnCount = 0;
    int barrelUsedCount = 0;
    bool strongPushUpgraded = false;

    [[nodiscard]] bool isAlive() const noexcept;
    void takeDamage(int damage) noexcept;
    void heal(int amount) noexcept;
};

} // namespace dungeon
