#pragma once

#include <array>

namespace dungeon {

inline constexpr int MAP_ROWS = 12;
inline constexpr int MAP_COLS = 20;
inline constexpr int TOTAL_LEVELS = 7;

struct Position {
    int row = 0;
    int col = 0;

    friend constexpr bool operator==(const Position& lhs, const Position& rhs) noexcept {
        return lhs.row == rhs.row && lhs.col == rhs.col;
    }

    friend constexpr bool operator!=(const Position& lhs, const Position& rhs) noexcept {
        return !(lhs == rhs);
    }
};

enum class Direction {
    Up,
    Down,
    Left,
    Right
};

inline constexpr bool isValidDirection(Direction direction) noexcept {
    return direction == Direction::Up || direction == Direction::Down ||
           direction == Direction::Left || direction == Direction::Right;
}

inline constexpr Position directionOffset(Direction direction) noexcept {
    switch (direction) {
    case Direction::Up:
        return {-1, 0};
    case Direction::Down:
        return {1, 0};
    case Direction::Left:
        return {0, -1};
    case Direction::Right:
        return {0, 1};
    }
    return {0, 0};
}

enum class MonsterType {
    Normal,
    Armor,
    Bomber,
    Boss
};

enum class GameState {
    Menu,
    Help,
    Playing,
    LevelClear,
    Upgrade,
    GameOver,
    GameWin,
    Exit
};

enum class InputKey {
    None,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Attack,
    Restart,
    Help,
    Confirm,
    Escape,
    Upgrade1,
    Upgrade2,
    Upgrade3,
    Upgrade4,
    Upgrade5,
    Quit
};

enum class DamageSource {
    PlayerAttack,
    MonsterAttack,
    Wall,
    Spike,
    Fire,
    BarrelExplosion,
    BomberExplosion,
    Crush
};

inline constexpr bool isTrapDamage(DamageSource source) noexcept {
    return source == DamageSource::Wall || source == DamageSource::Spike ||
           source == DamageSource::Fire || source == DamageSource::BarrelExplosion ||
           source == DamageSource::BomberExplosion;
}

inline constexpr std::array<Position, 5> EXPLOSION_OFFSETS{{
    {0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}
}};

} // namespace dungeon
