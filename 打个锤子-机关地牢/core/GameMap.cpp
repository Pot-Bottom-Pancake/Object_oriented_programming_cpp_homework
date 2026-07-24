#include "GameMap.h"

#include <string_view>
#include <utility>

namespace dungeon {
namespace {

constexpr std::string_view ALLOWED_CELLS = "#.PMAXDTFBHE";

void setError(std::wstring* error, std::wstring message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
}

} // namespace

bool GameMap::load(const std::vector<std::string>& rows, std::wstring* error) {
    if (rows.size() != static_cast<std::size_t>(MAP_ROWS)) {
        setError(error, L"地图行数必须为 12。");
        return false;
    }

    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].size() != static_cast<std::size_t>(MAP_COLS)) {
            setError(error, L"地图第 " + std::to_wstring(row + 1) + L" 行列数不是 20。");
            return false;
        }
        for (char cell : rows[row]) {
            if (ALLOWED_CELLS.find(cell) == std::string_view::npos) {
                setError(error, L"地图包含非法字符。");
                return false;
            }
        }
    }

    terrain_ = rows;
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

char GameMap::getCell(int row, int col) const noexcept {
    if (!inBounds(row, col) || terrain_.size() != static_cast<std::size_t>(MAP_ROWS)) {
        return '#';
    }
    return terrain_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

bool GameMap::setCell(int row, int col, char cell) noexcept {
    if (!inBounds(row, col) || terrain_.size() != static_cast<std::size_t>(MAP_ROWS) ||
        ALLOWED_CELLS.find(cell) == std::string_view::npos) {
        return false;
    }
    terrain_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = cell;
    return true;
}

bool GameMap::inBounds(int row, int col) const noexcept {
    return row >= 0 && row < MAP_ROWS && col >= 0 && col < MAP_COLS;
}

bool GameMap::isWall(int row, int col) const noexcept {
    return getCell(row, col) == '#';
}

bool GameMap::isSpike(int row, int col) const noexcept {
    return getCell(row, col) == 'T';
}

bool GameMap::isFire(int row, int col) const noexcept {
    return getCell(row, col) == 'F';
}

bool GameMap::isBarrel(int row, int col) const noexcept {
    return getCell(row, col) == 'B';
}

bool GameMap::isExit(int row, int col) const noexcept {
    return getCell(row, col) == 'E';
}

bool GameMap::isHeal(int row, int col) const noexcept {
    return getCell(row, col) == 'H';
}

const std::vector<std::string>& GameMap::terrain() const noexcept {
    return terrain_;
}

} // namespace dungeon
