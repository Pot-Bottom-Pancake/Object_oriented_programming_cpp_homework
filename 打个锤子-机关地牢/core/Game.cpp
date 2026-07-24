#include "Game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace dungeon {
namespace {

std::wstring positionText(Position position) {
    return L"(" + std::to_wstring(position.row) + L", " + std::to_wstring(position.col) + L")";
}

} // namespace

Game::Game(std::vector<LevelData::Level> levels)
    : levels_(std::move(levels)) {
    if (levels_.empty() || levels_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        levels_.clear();
        addLog(L"关卡数据无效，无法开始游戏。");
    }
}

void Game::startNewGame() {
    player_ = Player{};
    levelStartPlayer_ = player_;
    monsters_.clear();
    logs_.clear();
    logEntries_.clear();
    visualEvents_.clear();
    currentLevel_ = 0;
    wallDamage_ = 2;
    spikeDamageToMonster_ = 5;
    spikeDamageToPlayer_ = 3;
    fireDamage_ = 2;
    barrelDamage_ = 6;
    bomberDamage_ = 5;

    if (!loadLevelInternal(1, true)) {
        state_ = GameState::Menu;
    }
}

void Game::restartCurrentLevel() {
    if (currentLevel_ <= 0 || currentLevel_ > totalLevels()) {
        addLog(L"当前关卡无效，无法重新开始。");
        return;
    }

    player_ = levelStartPlayer_;
    if (loadLevelInternal(currentLevel_, false)) {
        state_ = GameState::Playing;
        addLog(L"已重新开始当前关卡。");
    }
}

bool Game::loadLevel(int levelNumber) {
    return loadLevelInternal(levelNumber, true);
}

bool Game::loadLevelInternal(int levelNumber, bool captureSnapshot) {
    if (levelNumber < 1 || levelNumber > totalLevels()) {
        addLog(L"关卡编号越界，加载失败。");
        return false;
    }

    GameMap candidateMap;
    std::wstring error;
    const auto& level = levels_[static_cast<std::size_t>(levelNumber - 1)];
    if (!candidateMap.load(level, &error)) {
        addLog(L"关卡加载失败：" + error);
        return false;
    }

    std::vector<Monster> candidateMonsters;
    Position playerSpawn{};
    int playerSpawnCount = 0;
    int exitCount = 0;
    int nextMonsterId = 1;

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            const char cell = candidateMap.getCell(row, col);
            MonsterType monsterType = MonsterType::Normal;
            bool isMonster = true;
            switch (cell) {
            case 'P':
                playerSpawn = {row, col};
                ++playerSpawnCount;
                (void)candidateMap.setCell(row, col, '.');
                isMonster = false;
                break;
            case 'M':
                monsterType = MonsterType::Normal;
                break;
            case 'A':
                monsterType = MonsterType::Armor;
                break;
            case 'X':
                monsterType = MonsterType::Bomber;
                break;
            case 'D':
                monsterType = MonsterType::Boss;
                break;
            case 'E':
                ++exitCount;
                [[fallthrough]];
            default:
                isMonster = false;
                break;
            }

            if (isMonster) {
                candidateMonsters.emplace_back(monsterType, Position{row, col}, nextMonsterId);
                nextMonsterId = saturatedAdd(nextMonsterId, 1);
                (void)candidateMap.setCell(row, col, '.');
            }
        }
    }

    if (playerSpawnCount != 1 || exitCount != 1) {
        addLog(L"关卡加载失败：每关必须恰好有一个玩家出生点和一个出口。");
        return false;
    }

    map_ = std::move(candidateMap);
    monsters_ = std::move(candidateMonsters);
    player_.position = playerSpawn;
    player_.direction = Direction::Right;
    for (Monster& monster : monsters_) {
        monster.lastMoveTurn = player_.turnCount;
    }

    currentLevel_ = levelNumber;
    state_ = GameState::Playing;
    exitOpenedAnnounced_ = false;
    logs_.clear();
    logEntries_.clear();
    visualEvents_.clear();
    addLog(L"进入第 " + std::to_wstring(currentLevel_) + L" 关。", LogCategory::System);
    if (captureSnapshot) {
        levelStartPlayer_ = player_;
    }
    return true;
}

void Game::handleInput(InputKey key) {
    if (key == InputKey::None || state_ == GameState::Exit) {
        return;
    }
    if (key == InputKey::Quit) {
        state_ = GameState::Exit;
        return;
    }

    switch (state_) {
    case GameState::Menu:
        if (key == InputKey::Confirm) {
            startNewGame();
        } else if (key == InputKey::Help) {
            state_ = GameState::Help;
        } else if (key == InputKey::Escape) {
            state_ = GameState::Exit;
        }
        break;
    case GameState::Help:
        if (key == InputKey::Escape) {
            state_ = GameState::Menu;
        }
        break;
    case GameState::Playing:
        handlePlayingInput(key);
        break;
    case GameState::LevelClear:
        if (key == InputKey::Confirm) {
            goToNextLevelOrUpgrade();
        } else if (key == InputKey::Escape) {
            state_ = GameState::Menu;
        }
        break;
    case GameState::Upgrade:
        if (key >= InputKey::Upgrade1 && key <= InputKey::Upgrade5) {
            const int option = static_cast<int>(key) - static_cast<int>(InputKey::Upgrade1) + 1;
            applyUpgrade(option);
        } else if (key == InputKey::Escape) {
            state_ = GameState::Menu;
        }
        break;
    case GameState::GameOver:
        if (key == InputKey::Restart) {
            restartCurrentLevel();
        } else if (key == InputKey::Escape) {
            state_ = GameState::Menu;
        }
        break;
    case GameState::GameWin:
        if (key == InputKey::Escape) {
            state_ = GameState::Menu;
        }
        break;
    case GameState::Exit:
        break;
    }
}

void Game::handlePlayingInput(InputKey key) {
    bool actionTaken = false;
    switch (key) {
    case InputKey::MoveUp:
        actionTaken = movePlayer(Direction::Up);
        break;
    case InputKey::MoveDown:
        actionTaken = movePlayer(Direction::Down);
        break;
    case InputKey::MoveLeft:
        actionTaken = movePlayer(Direction::Left);
        break;
    case InputKey::MoveRight:
        actionTaken = movePlayer(Direction::Right);
        break;
    case InputKey::Attack:
        playerAttack();
        actionTaken = true;
        break;
    case InputKey::Restart:
        restartCurrentLevel();
        return;
    case InputKey::Escape:
        state_ = GameState::Menu;
        return;
    default:
        return;
    }

    if (actionTaken) {
        finishPlayerAction();
    }
}

bool Game::movePlayer(Direction direction) {
    if (state_ != GameState::Playing || !player_.isAlive() || !isValidDirection(direction)) {
        return false;
    }

    player_.direction = direction;
    const Position offset = directionOffset(direction);
    const Position target{player_.position.row + offset.row, player_.position.col + offset.col};
    if (!canPlayerMoveTo(target.row, target.col)) {
        addLog(L"前方被挡住了。");
        return false;
    }

    const Position previous = player_.position;
    player_.position = target;
    VisualEvent moveEvent;
    moveEvent.type = VisualEventType::PlayerMoved;
    moveEvent.position = previous;
    moveEvent.targetPosition = target;
    moveEvent.direction = direction;
    addVisualEvent(std::move(moveEvent));
    applyEnterCellEffectToPlayer();
    return true;
}

bool Game::canPlayerMoveTo(int row, int col) const noexcept {
    if (!map_.inBounds(row, col) || map_.isWall(row, col) || map_.isBarrel(row, col)) {
        return false;
    }
    return !hasMonsterAt(row, col);
}

void Game::applyEnterCellEffectToPlayer() {
    const Position position = player_.position;
    if (map_.isHeal(position.row, position.col)) {
        const int before = player_.hp;
        player_.heal(HEAL_AMOUNT);
        const int healed = player_.hp - before;
        (void)map_.setCell(position.row, position.col, '.');
        addLog(L"拾取血瓶，恢复 " + std::to_wstring(healed) + L" 点生命。", LogCategory::Heal);
        VisualEvent healEvent;
        healEvent.type = VisualEventType::PlayerHealed;
        healEvent.position = position;
        healEvent.targetPosition = position;
        healEvent.value = healed;
        addVisualEvent(std::move(healEvent));
    } else if (map_.isSpike(position.row, position.col)) {
        VisualEvent spikeEvent;
        spikeEvent.type = VisualEventType::SpikeHit;
        spikeEvent.position = position;
        spikeEvent.targetPosition = position;
        spikeEvent.value = spikeDamageToPlayer_;
        spikeEvent.damageSource = DamageSource::Spike;
        addVisualEvent(std::move(spikeEvent));
        damagePlayer(spikeDamageToPlayer_, DamageSource::Spike, position);
        addLog(L"你踩上尖刺，受到 " + std::to_wstring(spikeDamageToPlayer_) + L" 点伤害。", LogCategory::Damage);
    } else if (map_.isExit(position.row, position.col) && !allMonstersDead()) {
        addLog(L"出口被魔法封锁了，必须先击败所有怪物！", LogCategory::Danger);
    }
}

void Game::playerAttack() {
    if (state_ != GameState::Playing || !player_.isAlive()) {
        return;
    }

    const Position offset = directionOffset(player_.direction);
    const Position target{player_.position.row + offset.row, player_.position.col + offset.col};
    VisualEvent attackEvent;
    attackEvent.type = VisualEventType::PlayerAttack;
    attackEvent.position = player_.position;
    attackEvent.targetPosition = target;
    attackEvent.direction = player_.direction;
    addVisualEvent(std::move(attackEvent));

    const int monsterIndex = findMonsterAt(target.row, target.col);
    if (monsterIndex >= 0) {
        const std::size_t index = static_cast<std::size_t>(monsterIndex);
        addLog(L"你挥动锤子，击中了" + std::wstring(monsters_[index].displayName()) + L"！", LogCategory::PlayerAction);
        damageMonster(index, player_.attack, DamageSource::PlayerAttack);
        for (int step = 0; step < player_.pushPower && index < monsters_.size() && monsters_[index].isAlive(); ++step) {
            if (!tryPushMonsterOneStep(index, player_.direction, 0)) {
                break;
            }
        }
        return;
    }

    if (map_.isBarrel(target.row, target.col)) {
        addLog(L"你一锤引爆了炸药桶！", LogCategory::Explosion);
        explodeBarrel(target.row, target.col);
        return;
    }

    addLog(L"你挥出一锤，但没有击中目标。", LogCategory::PlayerAction);
}

bool Game::tryPushMonsterOneStep(std::size_t index, Direction direction, int depth) {
    if (index >= monsters_.size() || !monsters_[index].isAlive() || depth < 0) {
        return false;
    }

    const Position offset = directionOffset(direction);
    const Position target{
        monsters_[index].position.row + offset.row,
        monsters_[index].position.col + offset.col
    };

    if (!map_.inBounds(target.row, target.col) || map_.isWall(target.row, target.col)) {
        addLog(std::wstring(monsters_[index].displayName()) + L"撞上墙壁，受到 " +
               std::to_wstring(wallDamage_) + L" 点伤害！", LogCategory::Damage);
        VisualEvent wallEvent;
        wallEvent.type = VisualEventType::WallHit;
        wallEvent.position = monsters_[index].position;
        wallEvent.targetPosition = target;
        wallEvent.value = wallDamage_;
        wallEvent.entityId = monsters_[index].id;
        wallEvent.direction = direction;
        wallEvent.monsterType = monsters_[index].type;
        wallEvent.damageSource = DamageSource::Wall;
        addVisualEvent(std::move(wallEvent));
        damageMonster(index, wallDamage_, DamageSource::Wall);
        return false;
    }

    if (map_.isExit(target.row, target.col)) {
        addLog(L"出口挡住了怪物的击退路线。");
        return false;
    }

    if (map_.isBarrel(target.row, target.col)) {
        addLog(std::wstring(monsters_[index].displayName()) + L"撞爆了炸药桶！", LogCategory::Explosion);
        explodeBarrel(target.row, target.col);
        return false;
    }

    const int blockingMonster = findMonsterAt(target.row, target.col);
    if (blockingMonster >= 0) {
        const std::size_t blockingIndex = static_cast<std::size_t>(blockingMonster);
        bool moved = false;
        if (depth < MAX_CHAIN_DEPTH) {
            moved = tryPushMonsterOneStep(blockingIndex, direction, depth + 1);
        }

        if (!moved) {
            addLog(L"怪物挤在一起，各受到 1 点挤压伤害。", LogCategory::Damage);
            damageMonster(index, 1, DamageSource::Crush);
            damageMonster(blockingIndex, 1, DamageSource::Crush);
            return false;
        }

        if (!monsters_[index].isAlive() || hasMonsterAt(target.row, target.col)) {
            return false;
        }
    }

    const Position previous = monsters_[index].position;
    monsters_[index].position = target;
    // 被成功击退的怪物本回合不立即走回原格，否则固定关卡的连续推怪解谜无法成立。
    monsters_[index].lastMoveTurn = saturatedAdd(player_.turnCount, 1);
    addLog(std::wstring(monsters_[index].displayName()) + L"被击退到 " + positionText(target) + L"。", LogCategory::PlayerAction);
    VisualEvent pushEvent;
    pushEvent.type = VisualEventType::MonsterPushed;
    pushEvent.position = previous;
    pushEvent.targetPosition = target;
    pushEvent.entityId = monsters_[index].id;
    pushEvent.direction = direction;
    pushEvent.monsterType = monsters_[index].type;
    addVisualEvent(std::move(pushEvent));
    applyEnterCellEffectToMonster(index);
    return true;
}

void Game::applyEnterCellEffectToMonster(std::size_t index) {
    if (index >= monsters_.size() || !monsters_[index].isAlive()) {
        return;
    }
    const Position position = monsters_[index].position;
    if (map_.isSpike(position.row, position.col)) {
        addLog(std::wstring(monsters_[index].displayName()) + L"撞上尖刺，受到 " +
               std::to_wstring(spikeDamageToMonster_) + L" 点伤害！", LogCategory::Damage);
        VisualEvent spikeEvent;
        spikeEvent.type = VisualEventType::SpikeHit;
        spikeEvent.position = position;
        spikeEvent.targetPosition = position;
        spikeEvent.value = spikeDamageToMonster_;
        spikeEvent.entityId = monsters_[index].id;
        spikeEvent.monsterType = monsters_[index].type;
        spikeEvent.damageSource = DamageSource::Spike;
        addVisualEvent(std::move(spikeEvent));
        damageMonster(index, spikeDamageToMonster_, DamageSource::Spike);
    }
}

void Game::finishPlayerAction() {
    if (player_.turnCount < std::numeric_limits<int>::max()) {
        ++player_.turnCount;
    }

    if (!player_.isAlive()) {
        setGameOver();
        return;
    }

    monsterTurn();
    if (!player_.isAlive()) {
        setGameOver();
        return;
    }

    applyFireDamage();
    if (!player_.isAlive()) {
        setGameOver();
        return;
    }

    checkLevelClear();
}

void Game::monsterTurn() {
    if (state_ != GameState::Playing) {
        return;
    }

    for (std::size_t index = 0; index < monsters_.size() && player_.isAlive(); ++index) {
        Monster& monster = monsters_[index];
        if (!monster.isAlive()) {
            continue;
        }

        const int elapsed = player_.turnCount >= monster.lastMoveTurn
                                ? player_.turnCount - monster.lastMoveTurn
                                : 0;
        if (elapsed < monster.moveInterval) {
            continue;
        }
        monster.lastMoveTurn = player_.turnCount;
        if (monster.consumeStunTurn()) {
            addLog(std::wstring(monster.displayName()) + L"仍在眩晕，本回合无法行动。",
                   LogCategory::PlayerAction);
            VisualEvent stunEvent;
            stunEvent.type = VisualEventType::MonsterStunned;
            stunEvent.position = monster.position;
            stunEvent.targetPosition = monster.position;
            stunEvent.entityId = monster.id;
            stunEvent.monsterType = monster.type;
            stunEvent.message = L"眩晕！";
            addVisualEvent(std::move(stunEvent));
            continue;
        }
        moveMonster(index);
    }
}

void Game::moveMonster(std::size_t index) {
    if (index >= monsters_.size() || !monsters_[index].isAlive() || !player_.isAlive()) {
        return;
    }

    Monster& monster = monsters_[index];
    const int rowDelta = player_.position.row - monster.position.row;
    const int colDelta = player_.position.col - monster.position.col;
    if (std::abs(rowDelta) + std::abs(colDelta) == 1) {
        const int damage = monster.attackDamage();
        damagePlayer(damage, DamageSource::MonsterAttack, player_.position);
        addLog(std::wstring(monster.displayName()) + L"攻击了你，造成 " + std::to_wstring(damage) + L" 点伤害。", LogCategory::Danger);
        return;
    }

    const Position vertical{rowDelta == 0 ? 0 : (rowDelta > 0 ? 1 : -1), 0};
    const Position horizontal{0, colDelta == 0 ? 0 : (colDelta > 0 ? 1 : -1)};
    const std::array<Position, 2> attempts = std::abs(rowDelta) > std::abs(colDelta)
                                                ? std::array<Position, 2>{vertical, horizontal}
                                                : std::array<Position, 2>{horizontal, vertical};

    for (const Position offset : attempts) {
        if (offset == Position{0, 0}) {
            continue;
        }
        const Position target{monster.position.row + offset.row, monster.position.col + offset.col};
        if (!canMonsterMoveTo(target.row, target.col)) {
            continue;
        }
        const Position previous = monster.position;
        monster.position = target;
        VisualEvent moveEvent;
        moveEvent.type = VisualEventType::MonsterMoved;
        moveEvent.position = previous;
        moveEvent.targetPosition = target;
        moveEvent.entityId = monster.id;
        moveEvent.monsterType = monster.type;
        addVisualEvent(std::move(moveEvent));
        applyEnterCellEffectToMonster(index);
        return;
    }
}

bool Game::canMonsterMoveTo(int row, int col) const noexcept {
    if (!map_.inBounds(row, col) || map_.isWall(row, col) || map_.isBarrel(row, col) ||
        map_.isExit(row, col) || player_.position == Position{row, col}) {
        return false;
    }
    return !hasMonsterAt(row, col);
}

void Game::applyFireDamage() {
    if (map_.isFire(player_.position.row, player_.position.col) && player_.isAlive()) {
        VisualEvent fireEvent;
        fireEvent.type = VisualEventType::FireDamage;
        fireEvent.position = player_.position;
        fireEvent.targetPosition = player_.position;
        fireEvent.value = fireDamage_;
        fireEvent.damageSource = DamageSource::Fire;
        addVisualEvent(std::move(fireEvent));
        damagePlayer(fireDamage_, DamageSource::Fire, player_.position);
        addLog(L"火坑灼烧了你，受到 " + std::to_wstring(fireDamage_) + L" 点伤害。", LogCategory::Fire);
    }

    std::vector<std::size_t> victims;
    victims.reserve(monsters_.size());
    for (std::size_t index = 0; index < monsters_.size(); ++index) {
        if (monsters_[index].isAlive() &&
            map_.isFire(monsters_[index].position.row, monsters_[index].position.col)) {
            victims.push_back(index);
        }
    }
    for (const std::size_t index : victims) {
        if (index < monsters_.size() && monsters_[index].isAlive()) {
            addLog(std::wstring(monsters_[index].displayName()) + L"受到火坑的 " +
                   std::to_wstring(fireDamage_) + L" 点伤害。", LogCategory::Fire);
            VisualEvent fireEvent;
            fireEvent.type = VisualEventType::FireDamage;
            fireEvent.position = monsters_[index].position;
            fireEvent.targetPosition = monsters_[index].position;
            fireEvent.value = fireDamage_;
            fireEvent.entityId = monsters_[index].id;
            fireEvent.monsterType = monsters_[index].type;
            fireEvent.damageSource = DamageSource::Fire;
            addVisualEvent(std::move(fireEvent));
            damageMonster(index, fireDamage_, DamageSource::Fire);
        }
    }
}

void Game::explodeBarrel(int row, int col) {
    if (!map_.inBounds(row, col) || !map_.isBarrel(row, col)) {
        return;
    }

    (void)map_.setCell(row, col, '.');
    player_.barrelUsedCount = saturatedAdd(player_.barrelUsedCount, 1);
    addLog(L"炸药桶在 " + positionText({row, col}) + L" 爆炸！", LogCategory::Explosion);
    VisualEvent explosionEvent;
    explosionEvent.type = VisualEventType::BarrelExplode;
    explosionEvent.position = {row, col};
    explosionEvent.targetPosition = {row, col};
    explosionEvent.value = barrelDamage_;
    explosionEvent.damageSource = DamageSource::BarrelExplosion;
    addVisualEvent(std::move(explosionEvent));

    for (const Position offset : EXPLOSION_OFFSETS) {
        const Position target{row + offset.row, col + offset.col};
        if (!map_.inBounds(target.row, target.col)) {
            continue;
        }
        if (player_.isAlive() && player_.position == target) {
            damagePlayer(barrelDamage_, DamageSource::BarrelExplosion, target);
            addLog(L"你被炸药桶波及，受到 " + std::to_wstring(barrelDamage_) + L" 点伤害。", LogCategory::Explosion);
        }
        const int monsterIndex = findMonsterAt(target.row, target.col);
        if (monsterIndex >= 0) {
            damageMonster(static_cast<std::size_t>(monsterIndex), barrelDamage_, DamageSource::BarrelExplosion);
        }
        if (map_.isBarrel(target.row, target.col)) {
            explodeBarrel(target.row, target.col);
        }
    }
}

void Game::explodeBomber(int row, int col, int sourceEntityId) {
    if (!map_.inBounds(row, col)) {
        return;
    }
    addLog(L"爆爆怪在 " + positionText({row, col}) + L" 发生死亡爆炸！", LogCategory::Explosion);
    VisualEvent explosionEvent;
    explosionEvent.type = VisualEventType::BomberExplode;
    explosionEvent.position = {row, col};
    explosionEvent.targetPosition = {row, col};
    explosionEvent.value = bomberDamage_;
    explosionEvent.entityId = sourceEntityId;
    explosionEvent.monsterType = MonsterType::Bomber;
    explosionEvent.damageSource = DamageSource::BomberExplosion;
    addVisualEvent(std::move(explosionEvent));
    for (const Position offset : EXPLOSION_OFFSETS) {
        const Position target{row + offset.row, col + offset.col};
        if (!map_.inBounds(target.row, target.col)) {
            continue;
        }
        if (player_.isAlive() && player_.position == target) {
            damagePlayer(bomberDamage_, DamageSource::BomberExplosion, target);
            addLog(L"你被爆爆怪波及，受到 " + std::to_wstring(bomberDamage_) + L" 点伤害。", LogCategory::Explosion);
        }
        const int monsterIndex = findMonsterAt(target.row, target.col);
        if (monsterIndex >= 0) {
            damageMonster(static_cast<std::size_t>(monsterIndex), bomberDamage_, DamageSource::BomberExplosion);
        }
        if (map_.isBarrel(target.row, target.col)) {
            explodeBarrel(target.row, target.col);
        }
    }
}

void Game::damageMonster(std::size_t index, int damage, DamageSource source) {
    if (index >= monsters_.size() || damage <= 0 || !monsters_[index].alive || monsters_[index].hp <= 0) {
        return;
    }
    const int before = monsters_[index].hp;
    const bool wasRaging = monsters_[index].type == MonsterType::Boss &&
                           monsters_[index].hp < monsters_[index].maxHp / 2;
    monsters_[index].takeDamage(damage);
    const int actualDamage = before - monsters_[index].hp;

    // 只有玩家锤子的直接伤害会眩晕；陷阱、爆炸和挤压伤害不改变行动状态。
    if (source == DamageSource::PlayerAttack && actualDamage > 0 && monsters_[index].isAlive()) {
        monsters_[index].stunForOneTurn();
    }

    VisualEvent hitEvent;
    hitEvent.type = VisualEventType::MonsterHit;
    hitEvent.position = monsters_[index].position;
    hitEvent.targetPosition = monsters_[index].position;
    hitEvent.value = actualDamage;
    hitEvent.entityId = monsters_[index].id;
    hitEvent.monsterType = monsters_[index].type;
    hitEvent.damageSource = source;
    addVisualEvent(std::move(hitEvent));

    const bool isRaging = monsters_[index].type == MonsterType::Boss && monsters_[index].hp > 0 &&
                          monsters_[index].hp < monsters_[index].maxHp / 2;
    if (!wasRaging && isRaging) {
        VisualEvent rageEvent;
        rageEvent.type = VisualEventType::BossRage;
        rageEvent.position = monsters_[index].position;
        rageEvent.targetPosition = monsters_[index].position;
        rageEvent.entityId = monsters_[index].id;
        rageEvent.monsterType = MonsterType::Boss;
        rageEvent.message = L"BOSS 狂暴！";
        addVisualEvent(std::move(rageEvent));
        addLog(L"BOSS 进入狂暴状态，攻击力提高！", LogCategory::Danger);
    }
    killMonsterIfNeeded(index, source);
}

void Game::damagePlayer(int damage, DamageSource source, Position effectPosition) {
    if (damage <= 0 || !player_.isAlive()) {
        return;
    }
    const int before = player_.hp;
    player_.takeDamage(damage);
    const int actualDamage = before - player_.hp;
    if (actualDamage <= 0) {
        return;
    }

    VisualEvent damageEvent;
    damageEvent.type = VisualEventType::PlayerDamaged;
    damageEvent.position = effectPosition;
    damageEvent.targetPosition = player_.position;
    damageEvent.value = actualDamage;
    damageEvent.damageSource = source;
    addVisualEvent(std::move(damageEvent));
}

void Game::killMonsterIfNeeded(std::size_t index, DamageSource source) {
    if (index >= monsters_.size() || !monsters_[index].alive || monsters_[index].hp > 0) {
        return;
    }

    Monster& monster = monsters_[index];
    const MonsterType type = monster.type;
    const Position deathPosition = monster.position;
    const std::wstring name(monster.displayName());
    monster.alive = false;
    monster.hp = 0;
    monster.stunTurnsRemaining = 0;
    player_.killCount = saturatedAdd(player_.killCount, 1);
    if (isTrapDamage(source)) {
        player_.trapKillCount = saturatedAdd(player_.trapKillCount, 1);
    }
    addLog(name + L"被击败了！", LogCategory::Success);

    VisualEvent deathEvent;
    deathEvent.type = VisualEventType::MonsterDeath;
    deathEvent.position = deathPosition;
    deathEvent.targetPosition = deathPosition;
    deathEvent.entityId = monster.id;
    deathEvent.monsterType = type;
    deathEvent.damageSource = source;
    addVisualEvent(std::move(deathEvent));

    if (type == MonsterType::Bomber) {
        explodeBomber(deathPosition.row, deathPosition.col, monster.id);
    }
    announceExitIfOpened();
}

int Game::findMonsterAt(int row, int col) const noexcept {
    for (std::size_t index = 0; index < monsters_.size(); ++index) {
        if (monsters_[index].isAlive() && monsters_[index].position == Position{row, col}) {
            if (index > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                return -1;
            }
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool Game::hasMonsterAt(int row, int col) const noexcept {
    return findMonsterAt(row, col) >= 0;
}

bool Game::allMonstersDead() const noexcept {
    return std::none_of(monsters_.begin(), monsters_.end(), [](const Monster& monster) {
        return monster.isAlive();
    });
}

int Game::aliveMonsterCount() const noexcept {
    int count = 0;
    for (const Monster& monster : monsters_) {
        if (monster.isAlive() && count < std::numeric_limits<int>::max()) {
            ++count;
        }
    }
    return count;
}

void Game::checkLevelClear() {
    if (state_ != GameState::Playing || !player_.isAlive() || !allMonstersDead() ||
        !map_.isExit(player_.position.row, player_.position.col)) {
        return;
    }

    if (currentLevel_ >= totalLevels()) {
        state_ = GameState::GameWin;
        addLog(L"机关地牢已被彻底攻破！", LogCategory::Success);
        VisualEvent winEvent;
        winEvent.type = VisualEventType::GameWin;
        winEvent.position = player_.position;
        winEvent.targetPosition = player_.position;
        winEvent.message = L"通关成功！";
        addVisualEvent(std::move(winEvent));
    } else if (isUpgradeLevel(currentLevel_)) {
        state_ = GameState::Upgrade;
        addLog(L"关卡通过，请选择一项升级。", LogCategory::Success);
        VisualEvent clearEvent;
        clearEvent.type = VisualEventType::LevelClear;
        clearEvent.position = player_.position;
        clearEvent.targetPosition = player_.position;
        clearEvent.message = L"关卡通过！";
        addVisualEvent(std::move(clearEvent));
    } else {
        state_ = GameState::LevelClear;
        addLog(L"第 " + std::to_wstring(currentLevel_) + L" 关通过！", LogCategory::Success);
        VisualEvent clearEvent;
        clearEvent.type = VisualEventType::LevelClear;
        clearEvent.position = player_.position;
        clearEvent.targetPosition = player_.position;
        clearEvent.message = L"关卡通过！";
        addVisualEvent(std::move(clearEvent));
    }
}

void Game::goToNextLevelOrUpgrade() {
    if (state_ != GameState::LevelClear) {
        return;
    }
    if (currentLevel_ >= totalLevels()) {
        state_ = GameState::GameWin;
        return;
    }
    (void)loadLevelInternal(currentLevel_ + 1, true);
}

void Game::applyUpgrade(int option) {
    if (state_ != GameState::Upgrade) {
        return;
    }

    std::wstring upgradeMessage;
    switch (option) {
    case 1:
        player_.maxHp = saturatedAdd(player_.maxHp, 4);
        player_.heal(4);
        upgradeMessage = L"升级成功：强壮体魄。";
        break;
    case 2:
        wallDamage_ = saturatedAdd(wallDamage_, 1);
        upgradeMessage = L"升级成功：重锤训练。";
        break;
    case 3:
        if (player_.strongPushUpgraded) {
            addLog(L"强力击退已达上限，请选择其他升级。", LogCategory::Danger);
            return;
        }
        player_.pushPower = 2;
        player_.strongPushUpgraded = true;
        upgradeMessage = L"升级成功：强力击退。";
        break;
    case 4:
        spikeDamageToMonster_ = saturatedAdd(spikeDamageToMonster_, 1);
        upgradeMessage = L"升级成功：尖刺熟练。";
        break;
    case 5:
        barrelDamage_ = saturatedAdd(barrelDamage_, 2);
        upgradeMessage = L"升级成功：爆破专家。";
        break;
    default:
        addLog(L"无效的升级选项。", LogCategory::Danger);
        return;
    }

    if (currentLevel_ >= totalLevels()) {
        state_ = GameState::GameWin;
        return;
    }
    if (loadLevelInternal(currentLevel_ + 1, true)) {
        addLog(upgradeMessage, LogCategory::Success);
        VisualEvent upgradeEvent;
        upgradeEvent.type = VisualEventType::UpgradeApplied;
        upgradeEvent.position = player_.position;
        upgradeEvent.targetPosition = player_.position;
        upgradeEvent.message = L"能力提升！";
        addVisualEvent(std::move(upgradeEvent));
        // 升级属性已是下一关的起始状态，需更新重开快照。
        levelStartPlayer_ = player_;
    }
}

void Game::setGameOver() {
    player_.hp = 0;
    state_ = GameState::GameOver;
    addLog(L"你倒在了机关地牢中……", LogCategory::Danger);
    VisualEvent gameOverEvent;
    gameOverEvent.type = VisualEventType::GameOver;
    gameOverEvent.position = player_.position;
    gameOverEvent.targetPosition = player_.position;
    gameOverEvent.message = L"你倒下了……";
    addVisualEvent(std::move(gameOverEvent));
}

void Game::addLog(std::wstring message, LogCategory category) {
    if (message.empty()) {
        return;
    }
    logs_.push_back(message);
    logEntries_.push_back(LogEntry{std::move(message), category});
    if (logs_.size() > static_cast<std::size_t>(MAX_LOGS)) {
        const auto removeCount = static_cast<std::ptrdiff_t>(logs_.size() - MAX_LOGS);
        logs_.erase(logs_.begin(), logs_.begin() + removeCount);
        logEntries_.erase(logEntries_.begin(), logEntries_.begin() + removeCount);
    }
}

void Game::announceExitIfOpened() {
    if (exitOpenedAnnounced_ || !allMonstersDead()) {
        return;
    }
    exitOpenedAnnounced_ = true;
    Position exitPosition = player_.position;
    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            if (map_.isExit(row, col)) {
                exitPosition = {row, col};
            }
        }
    }
    addLog(L"所有怪物已被击败，出口开启了！", LogCategory::Success);
    VisualEvent exitEvent;
    exitEvent.type = VisualEventType::ExitOpened;
    exitEvent.position = exitPosition;
    exitEvent.targetPosition = exitPosition;
    exitEvent.message = L"出口开启！";
    addVisualEvent(std::move(exitEvent));
}

void Game::addVisualEvent(VisualEvent event) {
    if (visualEvents_.size() >= MAX_VISUAL_EVENTS) {
        visualEvents_.erase(visualEvents_.begin());
    }
    visualEvents_.push_back(std::move(event));
}

bool Game::isUpgradeLevel(int levelNumber) noexcept {
    return levelNumber == 2 || levelNumber == 4 || levelNumber == 6;
}

int Game::saturatedAdd(int value, int increment) noexcept {
    if (increment > 0 && value > std::numeric_limits<int>::max() - increment) {
        return std::numeric_limits<int>::max();
    }
    if (increment < 0 && value < std::numeric_limits<int>::min() - increment) {
        return std::numeric_limits<int>::min();
    }
    return value + increment;
}

GameState Game::state() const noexcept {
    return state_;
}

const GameMap& Game::map() const noexcept {
    return map_;
}

const Player& Game::player() const noexcept {
    return player_;
}

const std::vector<Monster>& Game::monsters() const noexcept {
    return monsters_;
}

const std::vector<std::wstring>& Game::logs() const noexcept {
    return logs_;
}

const std::vector<LogEntry>& Game::logEntries() const noexcept {
    return logEntries_;
}

const std::vector<VisualEvent>& Game::visualEvents() const noexcept {
    return visualEvents_;
}

std::vector<VisualEvent> Game::takeVisualEvents() {
    std::vector<VisualEvent> result;
    result.swap(visualEvents_);
    return result;
}

int Game::currentLevel() const noexcept {
    return currentLevel_;
}

int Game::totalLevels() const noexcept {
    if (levels_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return 0;
    }
    return static_cast<int>(levels_.size());
}

int Game::wallDamage() const noexcept {
    return wallDamage_;
}

int Game::spikeDamageToMonster() const noexcept {
    return spikeDamageToMonster_;
}

int Game::spikeDamageToPlayer() const noexcept {
    return spikeDamageToPlayer_;
}

int Game::fireDamage() const noexcept {
    return fireDamage_;
}

int Game::barrelDamage() const noexcept {
    return barrelDamage_;
}

int Game::bomberDamage() const noexcept {
    return bomberDamage_;
}

} // namespace dungeon
