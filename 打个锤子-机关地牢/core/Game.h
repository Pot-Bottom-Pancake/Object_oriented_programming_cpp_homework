#pragma once

#include "GameMap.h"
#include "LevelData.h"
#include "Monster.h"
#include "Player.h"
#include "Types.h"
#include "VisualEvent.h"

#include <cstddef>
#include <string>
#include <vector>

namespace dungeon {

class Game {
public:
    explicit Game(std::vector<LevelData::Level> levels = LevelData::allLevels());

    void startNewGame();
    void restartCurrentLevel();
    [[nodiscard]] bool loadLevel(int levelNumber);
    void handleInput(InputKey key);

    [[nodiscard]] GameState state() const noexcept;
    [[nodiscard]] const GameMap& map() const noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] const std::vector<Monster>& monsters() const noexcept;
    [[nodiscard]] const std::vector<std::wstring>& logs() const noexcept;
    [[nodiscard]] const std::vector<LogEntry>& logEntries() const noexcept;
    [[nodiscard]] const std::vector<VisualEvent>& visualEvents() const noexcept;
    [[nodiscard]] std::vector<VisualEvent> takeVisualEvents();
    [[nodiscard]] int currentLevel() const noexcept;
    [[nodiscard]] int totalLevels() const noexcept;

    [[nodiscard]] int wallDamage() const noexcept;
    [[nodiscard]] int spikeDamageToMonster() const noexcept;
    [[nodiscard]] int spikeDamageToPlayer() const noexcept;
    [[nodiscard]] int fireDamage() const noexcept;
    [[nodiscard]] int barrelDamage() const noexcept;
    [[nodiscard]] int bomberDamage() const noexcept;

    [[nodiscard]] int findMonsterAt(int row, int col) const noexcept;
    [[nodiscard]] bool hasMonsterAt(int row, int col) const noexcept;
    [[nodiscard]] bool allMonstersDead() const noexcept;
    [[nodiscard]] int aliveMonsterCount() const noexcept;

    // 这些是完整的后端规则入口，UI 只能通过它们驱动游戏。
    [[nodiscard]] bool movePlayer(Direction direction);
    void playerAttack();
    void monsterTurn();
    void applyFireDamage();
    void explodeBarrel(int row, int col);
    void explodeBomber(int row, int col, int sourceEntityId = 0);
    void applyUpgrade(int option);
    void addLog(std::wstring message, LogCategory category = LogCategory::System);

private:
    static constexpr int MAX_LOGS = 4;
    static constexpr std::size_t MAX_VISUAL_EVENTS = 4096;
    static constexpr int MAX_CHAIN_DEPTH = 3;
    static constexpr int HEAL_AMOUNT = 6;

    GameState state_ = GameState::Menu;
    GameMap map_;
    Player player_;
    Player levelStartPlayer_;
    std::vector<Monster> monsters_;
    std::vector<std::wstring> logs_;
    std::vector<LogEntry> logEntries_;
    std::vector<VisualEvent> visualEvents_;
    std::vector<LevelData::Level> levels_;
    bool exitOpenedAnnounced_ = false;

    int currentLevel_ = 0;
    int wallDamage_ = 2;
    int spikeDamageToMonster_ = 5;
    int spikeDamageToPlayer_ = 3;
    int fireDamage_ = 2;
    int barrelDamage_ = 6;
    int bomberDamage_ = 5;

    [[nodiscard]] bool loadLevelInternal(int levelNumber, bool captureSnapshot);
    void handlePlayingInput(InputKey key);
    void finishPlayerAction();
    void setGameOver();

    [[nodiscard]] bool canPlayerMoveTo(int row, int col) const noexcept;
    [[nodiscard]] bool canMonsterMoveTo(int row, int col) const noexcept;
    void moveMonster(std::size_t index);

    [[nodiscard]] bool tryPushMonsterOneStep(std::size_t index, Direction direction, int depth);
    void applyEnterCellEffectToPlayer();
    void applyEnterCellEffectToMonster(std::size_t index);

    void damageMonster(std::size_t index, int damage, DamageSource source);
    void damagePlayer(int damage, DamageSource source, Position effectPosition);
    void killMonsterIfNeeded(std::size_t index, DamageSource source);
    void announceExitIfOpened();
    void addVisualEvent(VisualEvent event);
    void checkLevelClear();
    void goToNextLevelOrUpgrade();

    [[nodiscard]] static bool isUpgradeLevel(int levelNumber) noexcept;
    [[nodiscard]] static int saturatedAdd(int value, int increment) noexcept;
};

} // namespace dungeon
