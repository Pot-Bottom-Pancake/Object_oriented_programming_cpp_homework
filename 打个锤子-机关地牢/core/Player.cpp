#include "Player.h"

#include <algorithm>

namespace dungeon {

bool Player::isAlive() const noexcept {
    return hp > 0;
}

void Player::takeDamage(int damage) noexcept {
    if (damage <= 0 || !isAlive()) {
        return;
    }
    hp = std::max(0, hp - damage);
}

void Player::heal(int amount) noexcept {
    if (amount <= 0 || !isAlive()) {
        return;
    }
    hp = std::min(maxHp, hp + amount);
}

} // namespace dungeon
