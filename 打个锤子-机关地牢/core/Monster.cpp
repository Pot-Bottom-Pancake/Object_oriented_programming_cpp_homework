#include "Monster.h"

#include <algorithm>

namespace dungeon {

Monster::Monster(MonsterType monsterType, Position spawnPosition, int entityId)
    : position(spawnPosition), id(entityId), type(monsterType) {
    alive = false;
    switch (type) {
    case MonsterType::Normal:
        hp = maxHp = 6;
        attack = 2;
        moveInterval = 1;
        break;
    case MonsterType::Armor:
        hp = maxHp = 12;
        attack = 3;
        moveInterval = 2;
        break;
    case MonsterType::Bomber:
        hp = maxHp = 4;
        attack = 1;
        moveInterval = 1;
        break;
    case MonsterType::Boss:
        hp = maxHp = 28;
        attack = 4;
        moveInterval = 1;
        break;
    }
    alive = maxHp > 0;
}

bool Monster::isAlive() const noexcept {
    return alive && hp > 0;
}

void Monster::takeDamage(int damage) noexcept {
    if (damage <= 0 || !isAlive()) {
        return;
    }
    hp = std::max(0, hp - damage);
}

void Monster::stunForOneTurn() noexcept {
    if (isAlive()) {
        stunTurnsRemaining = 1;
    }
}

bool Monster::isStunned() const noexcept {
    return isAlive() && stunTurnsRemaining > 0;
}

bool Monster::consumeStunTurn() noexcept {
    if (!isStunned()) {
        return false;
    }
    --stunTurnsRemaining;
    return true;
}

int Monster::attackDamage() const noexcept {
    if (type == MonsterType::Boss && hp < maxHp / 2) {
        return 5;
    }
    return attack;
}

wchar_t Monster::symbol() const noexcept {
    switch (type) {
    case MonsterType::Normal:
        return L'M';
    case MonsterType::Armor:
        return L'A';
    case MonsterType::Bomber:
        return L'X';
    case MonsterType::Boss:
        return L'D';
    }
    return L'?';
}

std::wstring_view Monster::displayName() const noexcept {
    switch (type) {
    case MonsterType::Normal:
        return L"普通怪";
    case MonsterType::Armor:
        return L"铁甲怪";
    case MonsterType::Bomber:
        return L"爆爆怪";
    case MonsterType::Boss:
        return L"机关守卫";
    }
    return L"未知怪物";
}

} // namespace dungeon
