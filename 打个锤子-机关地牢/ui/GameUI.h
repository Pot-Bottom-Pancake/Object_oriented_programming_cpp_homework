#pragma once

#include "../core/Game.h"
#include "AnimationManager.h"

#include <cstdint>
#include <vector>

namespace dungeon {

class GameUI {
public:
    static constexpr int WINDOW_WIDTH = 1000;
    static constexpr int WINDOW_HEIGHT = 650;
    static constexpr int CELL_SIZE = 40;
    static constexpr int MAP_PIXEL_WIDTH = MAP_COLS * CELL_SIZE;
    static constexpr int MAP_PIXEL_HEIGHT = MAP_ROWS * CELL_SIZE;

    GameUI() = default;
    ~GameUI();

    GameUI(const GameUI&) = delete;
    GameUI& operator=(const GameUI&) = delete;

    void initWindow();
    void closeWindow();
    void consumeEvents(const std::vector<VisualEvent>& events);
    void draw(const Game& game);
    [[nodiscard]] InputKey getInput() const;

private:
    bool initialized_ = false;
    std::uint64_t frameCount_ = 0;
    double displayedHp_ = -1.0;
    int displayedHpLevel_ = 0;
    GameState lastState_ = GameState::Menu;
    AnimationManager animations_;

    void drawMenu() const;
    void drawHelp() const;
    void drawGame(const Game& game);
    void drawLevelClear(const Game& game);
    void drawUpgrade(const Game& game) const;
    void drawGameOver(const Game& game) const;
    void drawGameWin(const Game& game) const;

    void drawMap(const Game& game, int offsetX, int offsetY) const;
    void drawFloor(int left, int top, int row, int col) const;
    void drawTerrainCell(char cell, int row, int col, bool exitOpen, int offsetX, int offsetY) const;
    void drawMonster(const Monster& monster, int offsetX, int offsetY) const;
    void drawPlayer(const Player& player, int offsetX, int offsetY) const;
    void drawHUD(const Game& game);
    void drawLogs(const Game& game) const;
};

} // namespace dungeon
