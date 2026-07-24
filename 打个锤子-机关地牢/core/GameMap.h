#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace dungeon {

class GameMap {
public:
    [[nodiscard]] bool load(const std::vector<std::string>& rows, std::wstring* error = nullptr);

    [[nodiscard]] char getCell(int row, int col) const noexcept;
    [[nodiscard]] bool setCell(int row, int col, char cell) noexcept;
    [[nodiscard]] bool inBounds(int row, int col) const noexcept;

    [[nodiscard]] bool isWall(int row, int col) const noexcept;
    [[nodiscard]] bool isSpike(int row, int col) const noexcept;
    [[nodiscard]] bool isFire(int row, int col) const noexcept;
    [[nodiscard]] bool isBarrel(int row, int col) const noexcept;
    [[nodiscard]] bool isExit(int row, int col) const noexcept;
    [[nodiscard]] bool isHeal(int row, int col) const noexcept;
    [[nodiscard]] const std::vector<std::string>& terrain() const noexcept;

private:
    std::vector<std::string> terrain_;
};

} // namespace dungeon
