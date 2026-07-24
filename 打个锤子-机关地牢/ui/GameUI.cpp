#include "GameUI.h"

#include <graphics.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace dungeon {
namespace {

void useFont(int height, COLORREF color = WHITE, int weight = FW_NORMAL) {
    settextstyle(height, 0, L"Microsoft YaHei UI", 0, 0, weight, false, false, false);
    settextcolor(color);
    setbkmode(TRANSPARENT);
}

COLORREF dynamicRgb(int red, int green, int blue) {
    return RGB(static_cast<unsigned char>(std::clamp(red, 0, 255)),
               static_cast<unsigned char>(std::clamp(green, 0, 255)),
               static_cast<unsigned char>(std::clamp(blue, 0, 255)));
}

void drawCenteredText(int centerX, int y, const std::wstring& text) {
    outtextxy(centerX - textwidth(text.c_str()) / 2, y, text.c_str());
}

void drawCenteredTextShadow(int centerX, int y, const std::wstring& text,
                            COLORREF foreground, COLORREF shadow = RGB(13, 12, 18)) {
    const int left = centerX - textwidth(text.c_str()) / 2;
    settextcolor(shadow);
    outtextxy(left + 3, y + 3, text.c_str());
    settextcolor(foreground);
    outtextxy(left, y, text.c_str());
}

std::wstring fitTextToWidth(const std::wstring& text, int maxWidth) {
    if (maxWidth <= 0 || text.empty() || textwidth(text.c_str()) <= maxWidth) {
        return maxWidth <= 0 ? std::wstring{} : text;
    }
    std::wstring result = text;
    constexpr const wchar_t* ellipsis = L"…";
    while (!result.empty()) {
        result.pop_back();
        const std::wstring candidate = result + ellipsis;
        if (textwidth(candidate.c_str()) <= maxWidth) {
            return candidate;
        }
    }
    return {};
}

void drawPanel(int left, int top, int right, int bottom, COLORREF fill, COLORREF border, int radius = 10) {
    (void)radius;
    // 像素面板：硬边、两级描边、切角，不使用圆角和抗锯齿轮廓。
    setfillcolor(RGB(10, 13, 20));
    solidrectangle(left + 4, top + 4, right + 4, bottom + 4);
    setfillcolor(RGB(28, 32, 39));
    solidrectangle(left, top, right, bottom);
    setfillcolor(border);
    solidrectangle(left + 2, top + 2, right - 2, bottom - 2);
    setfillcolor(RGB(87, 91, 99));
    solidrectangle(left + 4, top + 4, right - 4, top + 6);
    solidrectangle(left + 4, top + 4, left + 6, bottom - 4);
    setfillcolor(RGB(27, 30, 38));
    solidrectangle(left + 4, bottom - 7, right - 4, bottom - 4);
    solidrectangle(right - 7, top + 4, right - 4, bottom - 4);
    setfillcolor(fill);
    solidrectangle(left + 7, top + 7, right - 7, bottom - 7);

    // 做旧石板裂纹，位置由面板坐标确定，保持逐帧稳定。
    if (right - left > 130 && bottom - top > 54) {
        const int crackX = left + 23 + (left + top) % std::max(1, right - left - 70);
        const int crackY = top + 15;
        setfillcolor(RGB(18, 22, 29));
        solidrectangle(crackX, crackY, crackX + 14, crackY + 1);
        solidrectangle(crackX + 12, crackY + 2, crackX + 14, crackY + 8);
        solidrectangle(crackX + 14, crackY + 7, crackX + 22, crackY + 8);
    }

    setfillcolor(RGB(18, 21, 27));
    solidrectangle(left, top, left + 6, top + 6);
    solidrectangle(right - 6, top, right, top + 6);
    solidrectangle(left, bottom - 6, left + 6, bottom);
    solidrectangle(right - 6, bottom - 6, right, bottom);
    setfillcolor(RGB(139, 126, 87));
    solidrectangle(left + 8, top + 8, left + 11, top + 11);
    solidrectangle(right - 11, top + 8, right - 8, top + 11);
}

void drawVine(int x, int y, int length, bool mirror = false) {
    if (length <= 0) {
        return;
    }
    const int direction = mirror ? -1 : 1;
    setfillcolor(RGB(42, 78, 51));
    for (int step = 0; step < length; step += 6) {
        const int vineX = x + direction * ((step / 6) % 3) * 3;
        solidrectangle(vineX, y + step, vineX + 2, y + step + 7);
        if ((step / 6) % 2 == 0) {
            setfillcolor(RGB(62, 105, 63));
            const int leafStart = vineX + direction * 2;
            const int leafEnd = vineX + direction * 7;
            solidrectangle(std::min(leafStart, leafEnd), y + step + 2,
                           std::max(leafStart, leafEnd), y + step + 5);
            setfillcolor(RGB(42, 78, 51));
        }
    }
}

void drawBeveledButton(int left, int top, int right, int bottom, bool selected) {
    const COLORREF rim = selected ? RGB(201, 160, 67) : RGB(111, 117, 128);
    const COLORREF fill = selected ? RGB(48, 57, 69) : RGB(38, 45, 58);
    drawPanel(left, top, right, bottom, fill, rim, 0);
    setfillcolor(selected ? RGB(238, 194, 83) : RGB(135, 144, 159));
    solidrectangle(left + 12, top + 11, left + 16, bottom - 11);
    setfillcolor(selected ? RGB(85, 128, 103) : RGB(60, 72, 88));
    solidrectangle(right - 18, top + 11, right - 12, bottom - 11);
}

enum class HudIcon {
    Heart,
    Hammer,
    Push,
    Monster,
    Skull,
    Trap,
    Blast,
    Level,
    Turn
};

void drawHudIcon(HudIcon icon, int centerX, int centerY, COLORREF color) {
    setfillcolor(RGB(15, 18, 24));
    solidrectangle(centerX - 10, centerY - 10, centerX + 10, centerY + 10);
    setfillcolor(RGB(62, 68, 79));
    solidrectangle(centerX - 8, centerY - 8, centerX + 8, centerY + 8);
    setfillcolor(color);
    switch (icon) {
    case HudIcon::Heart:
        solidrectangle(centerX - 6, centerY - 4, centerX + 6, centerY + 3);
        solidrectangle(centerX - 4, centerY + 4, centerX + 4, centerY + 6);
        solidrectangle(centerX - 5, centerY - 6, centerX - 1, centerY - 2);
        solidrectangle(centerX + 1, centerY - 6, centerX + 5, centerY - 2);
        break;
    case HudIcon::Hammer:
        solidrectangle(centerX - 6, centerY - 6, centerX + 5, centerY - 2);
        solidrectangle(centerX - 1, centerY - 2, centerX + 2, centerY + 7);
        break;
    case HudIcon::Push:
        solidrectangle(centerX - 7, centerY - 2, centerX + 4, centerY + 2);
        solidrectangle(centerX + 2, centerY - 5, centerX + 7, centerY + 5);
        break;
    case HudIcon::Monster:
        solidrectangle(centerX - 6, centerY - 4, centerX + 6, centerY + 6);
        solidrectangle(centerX - 3, centerY - 7, centerX + 3, centerY - 3);
        setfillcolor(RGB(24, 27, 32));
        solidrectangle(centerX - 4, centerY - 1, centerX - 2, centerY + 2);
        solidrectangle(centerX + 2, centerY - 1, centerX + 4, centerY + 2);
        break;
    case HudIcon::Skull:
        solidrectangle(centerX - 6, centerY - 6, centerX + 6, centerY + 3);
        solidrectangle(centerX - 3, centerY + 3, centerX + 3, centerY + 7);
        setfillcolor(RGB(24, 27, 32));
        solidrectangle(centerX - 4, centerY - 2, centerX - 1, centerY + 1);
        solidrectangle(centerX + 1, centerY - 2, centerX + 4, centerY + 1);
        break;
    case HudIcon::Trap:
        for (int i = 0; i < 3; ++i) {
            POINT points[]{{centerX - 8 + i * 6, centerY + 6},
                           {centerX - 5 + i * 6, centerY - 6},
                           {centerX - 2 + i * 6, centerY + 6}};
            solidpolygon(points, 3);
        }
        break;
    case HudIcon::Blast:
        solidrectangle(centerX - 2, centerY - 8, centerX + 2, centerY + 8);
        solidrectangle(centerX - 8, centerY - 2, centerX + 8, centerY + 2);
        solidrectangle(centerX - 6, centerY - 6, centerX - 3, centerY - 3);
        solidrectangle(centerX + 3, centerY + 3, centerX + 6, centerY + 6);
        break;
    case HudIcon::Level:
        solidrectangle(centerX - 6, centerY + 3, centerX + 6, centerY + 6);
        solidrectangle(centerX - 3, centerY - 1, centerX + 6, centerY + 2);
        solidrectangle(centerX, centerY - 6, centerX + 6, centerY - 2);
        break;
    case HudIcon::Turn:
        solidrectangle(centerX - 6, centerY - 6, centerX + 3, centerY - 3);
        solidrectangle(centerX - 6, centerY - 6, centerX - 3, centerY + 5);
        solidrectangle(centerX - 6, centerY + 3, centerX + 5, centerY + 6);
        solidrectangle(centerX + 2, centerY, centerX + 6, centerY + 6);
        break;
    }
}

void drawSegmentedBar(int left, int top, int right, int bottom, double ratio,
                      COLORREF fill, COLORREF highlight) {
    const double safeRatio = std::clamp(ratio, 0.0, 1.0);
    setfillcolor(RGB(13, 16, 22));
    solidrectangle(left, top, right, bottom);
    setfillcolor(RGB(77, 81, 88));
    solidrectangle(left + 2, top + 2, right - 2, bottom - 2);
    setfillcolor(RGB(25, 28, 35));
    solidrectangle(left + 4, top + 4, right - 4, bottom - 4);
    const int fillRight = left + 4 + static_cast<int>(std::lround(static_cast<double>(right - left - 8) * safeRatio));
    if (fillRight > left + 4) {
        setfillcolor(fill);
        solidrectangle(left + 4, top + 4, std::min(fillRight, right - 4), bottom - 4);
        setfillcolor(highlight);
        solidrectangle(left + 6, top + 5, std::min(fillRight, right - 4), top + 7);
    }
    setfillcolor(RGB(17, 20, 27));
    for (int segment = left + 30; segment < right - 4; segment += 27) {
        solidrectangle(segment, top + 3, segment + 2, bottom - 3);
    }
}

void drawTorch(int centerX, int top, std::uint64_t frameCount) {
    const int phase = static_cast<int>((frameCount / 6U + static_cast<std::uint64_t>(centerX)) % 3U);
    setfillcolor(RGB(34, 27, 24));
    solidrectangle(centerX - 5, top + 26, centerX + 5, top + 63);
    setfillcolor(RGB(133, 82, 46));
    solidrectangle(centerX - 2, top + 29, centerX + 2, top + 61);
    setfillcolor(RGB(176, 55, 29));
    solidrectangle(centerX - 9, top + 10 + phase, centerX + 9, top + 30);
    setfillcolor(RGB(248, 132, 35));
    solidrectangle(centerX - 6, top + 5 - phase, centerX + 6, top + 25);
    setfillcolor(RGB(255, 225, 92));
    solidrectangle(centerX - 3, top + 10 - phase, centerX + 3, top + 23);
}

void drawBrickBackdrop(int width, int height, COLORREF baseColor) {
    setfillcolor(baseColor);
    solidrectangle(0, 0, width, height);
    const int brickWidth = 64;
    const int brickHeight = 32;
    for (int row = 0; row * brickHeight < height; ++row) {
        const int shift = row % 2 == 0 ? 0 : brickWidth / 2;
        const COLORREF joint = row % 3 == 0 ? RGB(52, 59, 74) : RGB(38, 44, 58);
        setfillcolor(joint);
        solidrectangle(0, row * brickHeight, width, row * brickHeight + 2);
        for (int x = -shift; x < width; x += brickWidth) {
            setfillcolor(RGB(13, 17, 25));
            solidrectangle(x, row * brickHeight, x + 2,
                           std::min(height, (row + 1) * brickHeight));
            setfillcolor(RGB(57, 64, 80));
            solidrectangle(x + 3, row * brickHeight + 3, x + brickWidth - 3, row * brickHeight + 5);
        }
    }
}

void drawHammerIcon(int centerX, int centerY, double scale) {
    const int handleLength = static_cast<int>(std::lround(36.0 * scale));
    const int handleWidth = std::max(3, static_cast<int>(std::lround(7.0 * scale)));
    const int headWidth = static_cast<int>(std::lround(42.0 * scale));
    const int headHeight = static_cast<int>(std::lround(18.0 * scale));
    setfillcolor(RGB(23, 20, 24));
    solidrectangle(centerX - handleWidth / 2 - 2, centerY,
                   centerX + handleWidth / 2 + 2, centerY + handleLength + 2);
    setfillcolor(RGB(151, 91, 48));
    solidrectangle(centerX - handleWidth / 2, centerY,
                   centerX + handleWidth / 2, centerY + handleLength);
    setfillcolor(RGB(22, 24, 29));
    solidrectangle(centerX - headWidth / 2 - 2, centerY - headHeight / 2 - 2,
                   centerX + headWidth / 2 + 2, centerY + headHeight / 2 + 2);
    setfillcolor(RGB(139, 151, 166));
    solidrectangle(centerX - headWidth / 2, centerY - headHeight / 2,
                   centerX + headWidth / 2, centerY + headHeight / 2);
    setfillcolor(RGB(218, 226, 232));
    solidrectangle(centerX - headWidth / 2 + 3, centerY - headHeight / 2 + 3,
                   centerX + headWidth / 2 - 3, centerY - headHeight / 2 + 5);
}

void drawOrientedPixelHammer(int centerX, int centerY, Direction direction) {
    const Position offset = directionOffset(direction);
    const int handX = centerX + offset.col * 9;
    const int handY = centerY + offset.row * 9;
    const int headX = centerX + offset.col * 19;
    const int headY = centerY + offset.row * 19;

    setlinecolor(RGB(24, 20, 24));
    setlinestyle(PS_SOLID, 5);
    line(handX, handY, headX, headY);
    setlinecolor(RGB(151, 91, 48));
    setlinestyle(PS_SOLID, 3);
    line(handX, handY, headX, headY);
    setlinestyle(PS_SOLID, 1);

    setfillcolor(RGB(20, 22, 27));
    if (offset.col != 0) {
        solidrectangle(headX - 4, headY - 10, headX + 5, headY + 10);
        setfillcolor(RGB(151, 164, 178));
        solidrectangle(headX - 2, headY - 8, headX + 3, headY + 8);
        setfillcolor(RGB(225, 232, 236));
        solidrectangle(headX - 1, headY - 6, headX, headY + 5);
    } else {
        solidrectangle(headX - 10, headY - 4, headX + 10, headY + 5);
        setfillcolor(RGB(151, 164, 178));
        solidrectangle(headX - 8, headY - 2, headX + 8, headY + 3);
        setfillcolor(RGB(225, 232, 236));
        solidrectangle(headX - 6, headY - 1, headX + 5, headY);
    }
}

COLORREF logColor(LogCategory category) {
    switch (category) {
    case LogCategory::System:
        return RGB(185, 197, 218);
    case LogCategory::PlayerAction:
        return RGB(235, 239, 246);
    case LogCategory::Damage:
        return RGB(245, 92, 98);
    case LogCategory::Fire:
        return RGB(255, 142, 45);
    case LogCategory::Explosion:
        return RGB(255, 208, 65);
    case LogCategory::Heal:
        return RGB(92, 224, 132);
    case LogCategory::Success:
        return RGB(250, 211, 85);
    case LogCategory::Danger:
        return RGB(255, 78, 82);
    }
    return WHITE;
}

std::wstring hpText(const Player& player) {
    return std::to_wstring(player.hp) + L" / " + std::to_wstring(player.maxHp);
}

bool hasRagingBoss(const Game& game) {
    return std::any_of(game.monsters().begin(), game.monsters().end(), [](const Monster& monster) {
        return monster.isAlive() && monster.type == MonsterType::Boss && monster.hp < monster.maxHp / 2;
    });
}

int gridCenter(double coordinate) {
    return static_cast<int>(std::lround(coordinate * 40.0 + 20.0));
}

} // namespace

GameUI::~GameUI() {
    closeWindow();
}

void GameUI::initWindow() {
    if (initialized_) {
        return;
    }
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    BeginBatchDraw();
    initialized_ = true;
}

void GameUI::closeWindow() {
    if (!initialized_) {
        return;
    }
    EndBatchDraw();
    closegraph();
    initialized_ = false;
}

void GameUI::consumeEvents(const std::vector<VisualEvent>& events) {
    animations_.consumeEvents(events);
}

void GameUI::draw(const Game& game) {
    if (!initialized_) {
        return;
    }

    if (game.state() != lastState_) {
        if (game.state() == GameState::Menu || game.state() == GameState::Help) {
            animations_.clear();
        }
        if (game.state() == GameState::Playing && lastState_ != GameState::Playing &&
            lastState_ != GameState::LevelClear) {
            displayedHp_ = -1.0;
        }
        lastState_ = game.state();
    }

    ++frameCount_;
    animations_.update();
    setbkcolor(RGB(18, 19, 26));
    cleardevice();

    switch (game.state()) {
    case GameState::Menu:
        drawMenu();
        break;
    case GameState::Help:
        drawHelp();
        break;
    case GameState::Playing:
        drawGame(game);
        break;
    case GameState::LevelClear:
        drawLevelClear(game);
        break;
    case GameState::Upgrade:
        drawUpgrade(game);
        animations_.drawEffects(0, 0);
        break;
    case GameState::GameOver:
        drawGameOver(game);
        animations_.drawEffects(0, 0);
        break;
    case GameState::GameWin:
        drawGameWin(game);
        animations_.drawEffects(0, 0);
        break;
    case GameState::Exit:
        break;
    }
    FlushBatchDraw();
}

void GameUI::drawMenu() const {
    drawBrickBackdrop(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(22, 27, 36));

    const int pulse = static_cast<int>((frameCount_ / 12U) % 4U);
    drawTorch(115, 88, frameCount_);
    drawTorch(885, 88, frameCount_ + 2U);
    drawPanel(145, 42, 855, 292, RGB(31, 36, 45),
              dynamicRgb(126 + pulse * 6, 113 + pulse * 4, 82), 0);
    drawVine(160, 48, 92);
    drawVine(840, 48, 92, true);

    useFont(52, RGB(237, 218, 164), FW_BOLD);
    drawCenteredTextShadow(WINDOW_WIDTH / 2, 73, L"打个锤子", RGB(237, 218, 164));
    useFont(32, RGB(173, 188, 205), FW_BOLD);
    drawCenteredTextShadow(WINDOW_WIDTH / 2, 137, L"机关地牢", RGB(173, 188, 205));

    drawHammerIcon(WINDOW_WIDTH / 2, 200, 0.88 + static_cast<double>(pulse) * 0.012);
    setfillcolor(RGB(73, 83, 97));
    solidrectangle(260, 259, 740, 262);
    setfillcolor(RGB(26, 30, 38));
    solidrectangle(310, 266, 690, 269);

    useFont(19, RGB(198, 207, 218), FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 301, L"把怪物推进机关，用地牢击败地牢");

    const std::array<std::pair<const wchar_t*, const wchar_t*>, 3> options{{
        {L"ENTER", L"开始游戏"}, {L"H", L"游戏说明"}, {L"ESC", L"退出游戏"}
    }};
    for (std::size_t i = 0; i < options.size(); ++i) {
        const int top = 347 + static_cast<int>(i) * 68;
        drawBeveledButton(285, top, 715, top + 52, i == 0);
        useFont(15, i == 0 ? RGB(250, 209, 99) : RGB(139, 164, 191), FW_BOLD);
        outtextxy(320, top + 16, options[i].first);
        useFont(20, i == 0 ? RGB(246, 239, 213) : RGB(215, 220, 228), FW_BOLD);
        drawCenteredText(535, top + 12, options[i].second);
        if (i == 0) {
            setfillcolor(RGB(244, 202, 87));
            POINT pointer[]{{300, top + 19}, {309, top + 26}, {300, top + 33}};
            solidpolygon(pointer, 3);
        }
    }

    // 缓慢漂浮的尘埃像素，强化旧地牢氛围。
    setfillcolor(RGB(104, 102, 87));
    for (int dust = 0; dust < 12; ++dust) {
        const int x = 70 + (dust * 83 + static_cast<int>((frameCount_ / 8U) % 860U)) % 860;
        const int y = 35 + (dust * 47 + static_cast<int>((frameCount_ / 13U) % 570U)) % 570;
        solidrectangle(x, y, x + (dust % 3 == 0 ? 2 : 1), y + 1);
    }
}

void GameUI::drawHelp() const {
    drawBrickBackdrop(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(24, 28, 36));
    useFont(38, RGB(249, 205, 78), FW_BOLD);
    drawCenteredTextShadow(WINDOW_WIDTH / 2, 28, L"机关猎人手册", RGB(234, 205, 130));

    drawPanel(55, 92, 475, 548, RGB(39, 43, 50), RGB(102, 113, 127), 15);
    drawPanel(525, 92, 945, 548, RGB(39, 43, 50), RGB(122, 105, 77), 15);
    drawVine(66, 100, 74);
    drawVine(934, 100, 74, true);

    useFont(24, RGB(111, 182, 255), FW_BOLD);
    drawCenteredText(265, 115, L"操作说明");
    const std::array<std::pair<const wchar_t*, const wchar_t*>, 7> controls{{
        {L"W / ↑", L"向上移动"}, {L"S / ↓", L"向下移动"},
        {L"A / ←", L"向左移动"}, {L"D / →", L"向右移动"},
        {L"J / SPACE", L"挥锤攻击"}, {L"R", L"重开当前关"},
        {L"ESC", L"返回主菜单"}
    }};
    for (std::size_t i = 0; i < controls.size(); ++i) {
        const int y = 165 + static_cast<int>(i) * 48;
        setfillcolor(i % 2 == 0 ? RGB(45, 50, 59) : RGB(36, 41, 49));
        solidrectangle(76, y - 6, 454, y + 28);
        useFont(17, RGB(123, 181, 244), FW_BOLD);
        outtextxy(85, y, controls[i].first);
        useFont(18, RGB(225, 230, 239));
        outtextxy(235, y - 1, controls[i].second);
    }

    useFont(24, RGB(255, 145, 82), FW_BOLD);
    drawCenteredText(735, 115, L"机关图鉴");
    const std::array<std::pair<const wchar_t*, const wchar_t*>, 5> traps{{
        {L"▲  尖刺", L"进入时造成高伤害"},
        {L"◉  火坑", L"回合末持续灼烧"},
        {L"▣  炸药桶", L"十字范围连锁爆炸"},
        {L"█  石墙", L"怪物撞墙会受伤"},
        {L"✚  血瓶", L"恢复 6 点生命"}
    }};
    for (std::size_t i = 0; i < traps.size(); ++i) {
        const int y = 168 + static_cast<int>(i) * 67;
        setfillcolor(i % 2 == 0 ? RGB(45, 50, 59) : RGB(36, 41, 49));
        solidrectangle(548, y - 8, 922, y + 48);
        useFont(20, i == 4 ? RGB(92, 225, 132) : RGB(244, 180, 90), FW_BOLD);
        outtextxy(562, y, traps[i].first);
        useFont(16, RGB(196, 203, 217));
        outtextxy(562, y + 29, traps[i].second);
    }

    useFont(18, RGB(250, 207, 76));
    drawCenteredText(WINDOW_WIDTH / 2, 585, L"锤击使怪物眩晕一次行动；未眩晕的贴身怪物会反击  ·  ESC 返回");
}

void GameUI::drawGame(const Game& game) {
    const auto [shakeX, shakeY] = animations_.screenShakeOffset();
    setfillcolor(RGB(15, 17, 23));
    solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    drawMap(game, shakeX, shakeY);
    animations_.drawEffects(shakeX, shakeY);
    drawHUD(game);
    drawLogs(game);
}

void GameUI::drawMap(const Game& game, int offsetX, int offsetY) const {
    const bool exitOpen = game.allMonstersDead();
    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            drawTerrainCell(game.map().getCell(row, col), row, col, exitOpen, offsetX, offsetY);
        }
    }
    for (const Monster& monster : game.monsters()) {
        if (monster.isAlive()) {
            drawMonster(monster, offsetX, offsetY);
        }
    }
    if (game.player().isAlive()) {
        drawPlayer(game.player(), offsetX, offsetY);
    }
}

void GameUI::drawFloor(int left, int top, int row, int col) const {
    const bool alternate = (row + col) % 2 == 0;
    setfillcolor(alternate ? RGB(48, 54, 70) : RGB(44, 50, 66));
    solidrectangle(left, top, left + CELL_SIZE, top + CELL_SIZE);
    setfillcolor(RGB(61, 68, 86));
    solidrectangle(left + 1, top + 1, left + CELL_SIZE - 1, top + 2);
    solidrectangle(left + 1, top + 1, left + 2, top + CELL_SIZE - 1);
    setfillcolor(RGB(31, 37, 50));
    solidrectangle(left + 1, top + CELL_SIZE - 2, left + CELL_SIZE - 1, top + CELL_SIZE - 1);
    solidrectangle(left + CELL_SIZE - 2, top + 1, left + CELL_SIZE - 1, top + CELL_SIZE - 1);

    if ((row * 7 + col * 11) % 4 == 0) {
        setfillcolor(RGB(70, 77, 94));
        solidrectangle(left + 7, top + 12, left + 17, top + 13);
        solidrectangle(left + 16, top + 10, left + 19, top + 12);
        solidrectangle(left + 18, top + 13, left + 24, top + 14);
    } else if ((row + col * 3) % 5 == 0) {
        setfillcolor(RGB(65, 72, 90));
        solidrectangle(left + 24, top + 25, left + 31, top + 26);
        solidrectangle(left + 30, top + 27, left + 34, top + 28);
    }
}

void GameUI::drawTerrainCell(char cell, int row, int col, bool exitOpen, int offsetX, int offsetY) const {
    const int left = col * CELL_SIZE + offsetX;
    const int top = row * CELL_SIZE + offsetY;
    const int right = left + CELL_SIZE;
    const int bottom = top + CELL_SIZE;
    drawFloor(left, top, row, col);

    switch (cell) {
    case '#':
        setfillcolor(RGB(19, 24, 34));
        solidrectangle(left, top, right, bottom);
        setfillcolor(RGB(64, 75, 96));
        solidrectangle(left + 2, top + 2, right - 3, bottom - 4);
        setfillcolor(RGB(91, 104, 128));
        solidrectangle(left + 4, top + 4, right - 5, top + 7);
        solidrectangle(left + 4, top + 7, left + 7, bottom - 8);
        setfillcolor(RGB(45, 54, 72));
        solidrectangle(left + 4, bottom - 10, right - 5, bottom - 5);
        setfillcolor(RGB(27, 34, 48));
        solidrectangle(left + 2, bottom - 4, right - 2, bottom - 1);
        solidrectangle(right - 5, top + 4, right - 2, bottom - 4);
        setfillcolor(RGB(39, 47, 64));
        solidrectangle(left + 3, top + 19, right - 4, top + 21);
        if ((row + col) % 2 == 0) {
            solidrectangle(left + 19, top + 3, left + 21, top + 20);
            solidrectangle(left + 8, top + 27, left + 25, top + 29);
        } else {
            solidrectangle(left + 11, top + 3, left + 13, top + 20);
            solidrectangle(left + 25, top + 21, left + 27, bottom - 5);
        }
        break;
    case 'T': {
        const bool triggered = animations_.isSpikeTriggered({row, col});
        const int idleLift = static_cast<int>((frameCount_ / 18U + static_cast<std::uint64_t>(row + col)) % 2U);
        const int lift = idleLift + (triggered ? 3 : 0);
        setfillcolor(RGB(35, 28, 34));
        solidrectangle(left + 3, bottom - 10, right - 3, bottom - 4);
        for (int spike = 0; spike < 3; ++spike) {
            const int x = left + 3 + spike * 12;
            POINT outline[]{{x, bottom - 8}, {x + 6, top + 7 - lift}, {x + 12, bottom - 8}};
            setfillcolor(RGB(18, 21, 28));
            solidpolygon(outline, 3);
            POINT points[]{{x + 2, bottom - 9}, {x + 6, top + 11 - lift}, {x + 10, bottom - 9}};
            setfillcolor(triggered ? RGB(229, 65, 72) : RGB(154, 164, 179));
            solidpolygon(points, 3);
            setfillcolor(triggered ? RGB(255, 129, 107) : RGB(224, 230, 235));
            solidrectangle(x + 5, top + 13 - lift, x + 6, bottom - 11);
        }
        setfillcolor(triggered ? RGB(139, 35, 45) : RGB(75, 82, 99));
        solidrectangle(left + 4, bottom - 8, right - 4, bottom - 5);
        break;
    }
    case 'F': {
        const int phase = static_cast<int>((frameCount_ / 6U + static_cast<std::uint64_t>(row * 3 + col)) % 3U);
        setfillcolor(RGB(20, 18, 24));
        solidrectangle(left + 4, top + 9, right - 4, bottom - 5);
        setfillcolor(RGB(92, 33, 31));
        solidrectangle(left + 7, top + 12, right - 7, bottom - 7);
        setfillcolor(RGB(228, 68, 25));
        solidrectangle(left + 8, top + 20 - phase * 2, left + 14, bottom - 8);
        solidrectangle(left + 15, top + 12 + phase * 2, left + 22, bottom - 8);
        solidrectangle(left + 23, top + 17 - phase, right - 8, bottom - 8);
        setfillcolor(RGB(255, 165, 31));
        solidrectangle(left + 12, top + 23 - phase, left + 18, bottom - 8);
        solidrectangle(left + 20, top + 20 + phase, left + 26, bottom - 8);
        setfillcolor(RGB(255, 225, 83));
        solidrectangle(left + 18, top + 26 - phase, left + 22, bottom - 8);
        break;
    }
    case 'B': {
        setfillcolor(RGB(25, 20, 22));
        solidrectangle(left + 6, top + 7, right - 6, bottom - 4);
        setfillcolor(RGB(112, 61, 37));
        solidrectangle(left + 9, top + 5, right - 9, bottom - 5);
        setfillcolor(RGB(174, 99, 48));
        solidrectangle(left + 11, top + 8, left + 15, bottom - 8);
        setfillcolor(RGB(169, 46, 43));
        solidrectangle(left + 7, top + 18, right - 7, top + 24);
        setfillcolor(RGB(222, 73, 53));
        solidrectangle(left + 10, top + 19, right - 10, top + 21);
        setfillcolor(RGB(27, 25, 28));
        solidrectangle(left + 18, top + 2, left + 21, top + 7);
        solidrectangle(left + 20, top, left + 27, top + 3);
        const bool spark = (frameCount_ / 7U + static_cast<std::uint64_t>(row + col)) % 2U == 0U;
        setfillcolor(spark ? RGB(255, 215, 70) : RGB(238, 90, 30));
        solidrectangle(left + 27, top, left + (spark ? 32 : 30), top + (spark ? 4 : 3));
        break;
    }
    case 'H': {
        const int pulse = static_cast<int>((frameCount_ / 10U + static_cast<std::uint64_t>(row + col)) % 4U);
        setfillcolor(dynamicRgb(38, 93 + pulse * 8, 70));
        solidrectangle(left + 7 - pulse / 2, top + 8 - pulse / 2,
                       right - 7 + pulse / 2, bottom - 4 + pulse / 2);
        setfillcolor(RGB(21, 26, 31));
        solidrectangle(left + 10, top + 9, right - 10, bottom - 5);
        setfillcolor(RGB(46, 166, 88));
        solidrectangle(left + 12, top + 13, right - 12, bottom - 7);
        setfillcolor(RGB(103, 218, 137));
        solidrectangle(left + 15, top + 7, right - 15, top + 13);
        setfillcolor(WHITE);
        solidrectangle(left + 18, top + 15, left + 22, bottom - 9);
        solidrectangle(left + 14, top + 20, right - 14, top + 24);
        break;
    }
    case 'E':
        if (exitOpen) {
            const int pulse = static_cast<int>((frameCount_ / 8U) % 5U);
            setfillcolor(dynamicRgb(46, 111 + pulse * 8, 125 + pulse * 8));
            solidrectangle(left + 4, top + 3, right - 4, bottom - 2);
            setfillcolor(RGB(237, 188, 52));
            solidrectangle(left + 7, top + 5, right - 7, bottom - 3);
            setfillcolor(RGB(38, 48, 58));
            solidrectangle(left + 11, top + 9, right - 11, bottom - 4);
            setfillcolor(RGB(63, 199, 207));
            solidrectangle(left + 14, top + 12, right - 14, bottom - 6);
            setfillcolor(RGB(178, 246, 229));
            solidrectangle(left + 16, top + 14, left + 18, bottom - 8);
        } else {
            setfillcolor(RGB(41, 37, 31));
            solidrectangle(left + 6, top + 4, right - 6, bottom - 3);
            setfillcolor(RGB(116, 92, 49));
            solidrectangle(left + 8, top + 6, right - 8, bottom - 4);
            setfillcolor(RGB(50, 49, 53));
            solidrectangle(left + 12, top + 10, right - 12, bottom - 5);
            setfillcolor(RGB(24, 27, 34));
            solidrectangle(left + 17, top + 18, right - 17, bottom - 9);
            solidrectangle(left + 14, top + 13, right - 14, top + 16);
        }
        break;
    default:
        break;
    }
}

void GameUI::drawMonster(const Monster& monster, int offsetX, int offsetY) const {
    const GridPoint drawPosition = animations_.monsterDrawPosition(monster.id, monster.position);
    const int centerX = gridCenter(drawPosition.col) + offsetX;
    int centerY = gridCenter(drawPosition.row) + offsetY;
    const bool flashing = animations_.isMonsterFlashing(monster.id);

    setfillcolor(RGB(22, 20, 28));
    solidrectangle(centerX - 14, centerY + 10, centerX + 14, centerY + 14);

    switch (monster.type) {
    case MonsterType::Normal: {
        const int bob = static_cast<int>((frameCount_ / 15U + static_cast<std::uint64_t>(monster.id)) % 2U);
        centerY -= bob;
        setfillcolor(RGB(20, 24, 22));
        solidrectangle(centerX - 15, centerY - 5, centerX + 15, centerY + 12);
        solidrectangle(centerX - 10, centerY - 11, centerX + 10, centerY + 13);
        setfillcolor(flashing ? WHITE : RGB(94, 218, 51));
        solidrectangle(centerX - 12, centerY - 4, centerX + 12, centerY + 10);
        solidrectangle(centerX - 8, centerY - 8, centerX + 8, centerY + 12);
        setfillcolor(flashing ? RGB(238, 242, 238) : RGB(124, 242, 64));
        solidrectangle(centerX - 7, centerY - 6, centerX + 5, centerY - 3);
        setfillcolor(RGB(25, 34, 27));
        solidrectangle(centerX - 7, centerY, centerX - 3, centerY + 5);
        solidrectangle(centerX + 4, centerY, centerX + 8, centerY + 5);
        setfillcolor(flashing ? RGB(220, 224, 220) : RGB(46, 151, 35));
        solidrectangle(centerX - 11, centerY + 8, centerX + 11, centerY + 11);
        break;
    }
    case MonsterType::Armor: {
        const bool glint = (frameCount_ / 28U + static_cast<std::uint64_t>(monster.id)) % 4U == 0U;
        setfillcolor(RGB(21, 22, 28));
        solidrectangle(centerX - 15, centerY - 11, centerX + 15, centerY + 13);
        solidrectangle(centerX - 10, centerY - 16, centerX + 10, centerY + 14);
        setfillcolor(flashing ? WHITE : (glint ? RGB(225, 231, 238) : RGB(156, 168, 184)));
        solidrectangle(centerX - 12, centerY - 9, centerX + 12, centerY + 11);
        solidrectangle(centerX - 8, centerY - 13, centerX + 8, centerY + 12);
        setfillcolor(RGB(61, 53, 82));
        solidrectangle(centerX - 9, centerY - 4, centerX + 9, centerY + 7);
        setfillcolor(flashing ? RGB(235, 235, 242) : RGB(128, 73, 190));
        solidrectangle(centerX - 7, centerY - 2, centerX + 7, centerY + 8);
        setfillcolor(RGB(26, 24, 32));
        solidrectangle(centerX - 5, centerY, centerX - 2, centerY + 5);
        solidrectangle(centerX + 3, centerY, centerX + 6, centerY + 5);
        setfillcolor(RGB(229, 233, 238));
        solidrectangle(centerX - 5, centerY - 13, centerX + 5, centerY - 11);
        break;
    }
    case MonsterType::Bomber: {
        const int pulse = static_cast<int>((frameCount_ / 12U + static_cast<std::uint64_t>(monster.id)) % 2U);
        setfillcolor(RGB(25, 22, 24));
        solidrectangle(centerX - 15 - pulse, centerY - 9 - pulse, centerX + 15 + pulse, centerY + 13 + pulse);
        solidrectangle(centerX - 10, centerY - 14 - pulse, centerX + 10, centerY + 14 + pulse);
        setfillcolor(flashing ? WHITE : RGB(240, 176, 35));
        solidrectangle(centerX - 12 - pulse, centerY - 7 - pulse, centerX + 12 + pulse, centerY + 11 + pulse);
        solidrectangle(centerX - 8, centerY - 11 - pulse, centerX + 8, centerY + 12 + pulse);
        setfillcolor(flashing ? RGB(240, 240, 240) : RGB(255, 214, 61));
        solidrectangle(centerX - 7, centerY - 8, centerX + 3, centerY - 5);
        setfillcolor(RGB(32, 28, 28));
        solidrectangle(centerX + 3, centerY - 16, centerX + 6, centerY - 11);
        solidrectangle(centerX + 5, centerY - 20, centerX + 12, centerY - 17);
        setfillcolor((frameCount_ / 5U) % 2U == 0U ? RGB(255, 225, 70) : RGB(245, 80, 25));
        solidrectangle(centerX + 11, centerY - 22, centerX + 15, centerY - 18);
        setfillcolor(RGB(71, 49, 25));
        solidrectangle(centerX - 2, centerY - 3, centerX + 2, centerY + 5);
        solidrectangle(centerX - 2, centerY + 8, centerX + 2, centerY + 10);
        break;
    }
    case MonsterType::Boss: {
        const bool raging = monster.hp < monster.maxHp / 2;
        if (raging) {
            const int ring = 19 + static_cast<int>((frameCount_ / 8U) % 3U);
            setfillcolor(RGB(122, 32, 43));
            solidrectangle(centerX - ring, centerY - ring, centerX + ring, centerY - ring + 2);
            solidrectangle(centerX - ring, centerY + ring - 2, centerX + ring, centerY + ring);
            solidrectangle(centerX - ring, centerY - ring, centerX - ring + 2, centerY + ring);
            solidrectangle(centerX + ring - 2, centerY - ring, centerX + ring, centerY + ring);
        }
        setfillcolor(RGB(24, 20, 25));
        solidrectangle(centerX - 18, centerY - 16, centerX + 18, centerY + 16);
        setfillcolor(flashing ? WHITE : (raging ? RGB(215, 46, 51) : RGB(145, 26, 36)));
        solidrectangle(centerX - 15, centerY - 13, centerX + 15, centerY + 13);
        setfillcolor(flashing ? RGB(245, 245, 245) : (raging ? RGB(242, 70, 63) : RGB(182, 41, 51)));
        solidrectangle(centerX - 10, centerY - 10, centerX + 10, centerY - 7);
        setfillcolor(raging ? RGB(255, 76, 70) : RGB(244, 190, 47));
        solidrectangle(centerX - 10, centerY - 4, centerX - 3, centerY);
        solidrectangle(centerX + 3, centerY - 4, centerX + 10, centerY);
        setfillcolor(RGB(35, 29, 34));
        solidrectangle(centerX - 5, centerY + 4, centerX + 5, centerY + 10);
        setfillcolor(raging ? RGB(255, 85, 75) : RGB(235, 182, 52));
        solidrectangle(centerX - 2, centerY + 5, centerX + 2, centerY + 8);
        break;
    }
    }

    const int barWidth = monster.type == MonsterType::Boss ? 36 : 30;
    const int barTop = centerY - (monster.type == MonsterType::Boss ? 24 : 20);
    const double ratio = monster.maxHp > 0
                             ? std::clamp(static_cast<double>(monster.hp) / static_cast<double>(monster.maxHp), 0.0, 1.0)
                             : 0.0;
    setfillcolor(RGB(35, 37, 43));
    solidrectangle(centerX - barWidth / 2, barTop, centerX + barWidth / 2, barTop + 4);
    setfillcolor(monster.type == MonsterType::Boss ? RGB(235, 62, 67) : RGB(83, 218, 111));
    solidrectangle(centerX - barWidth / 2, barTop,
                   centerX - barWidth / 2 + static_cast<int>(std::lround(static_cast<double>(barWidth) * ratio)),
                   barTop + 4);

    if (monster.isStunned()) {
        const int orbit = static_cast<int>((frameCount_ / 6U + static_cast<std::uint64_t>(monster.id)) % 3U);
        useFont(13, RGB(255, 221, 82), FW_BOLD);
        outtextxy(centerX - 15 + orbit * 3, barTop - 17, L"★");
        outtextxy(centerX + 5 - orbit * 2, barTop - 14, L"★");
    }
}

void GameUI::drawPlayer(const Player& player, int offsetX, int offsetY) const {
    const GridPoint drawPosition = animations_.playerDrawPosition(player.position);
    const int centerX = gridCenter(drawPosition.col) + offsetX;
    const int centerY = gridCenter(drawPosition.row) + offsetY;
    const int breath = static_cast<int>((frameCount_ / 20U) % 2U);
    const bool flashing = animations_.isPlayerFlashing();
    const Position facing = directionOffset(player.direction);

    setfillcolor(RGB(21, 20, 27));
    solidrectangle(centerX - 14, centerY + 10, centerX + 14, centerY + 14);

    // 披风位于身体后方，并随朝向略微偏移，增强角色轮廓和移动方向辨识度。
    const int capeX = centerX - facing.col * 3;
    const int capeY = centerY - facing.row * 2;
    setfillcolor(RGB(22, 21, 28));
    solidrectangle(capeX - 12, capeY - 1, capeX + 12, capeY + 13);
    setfillcolor(flashing ? WHITE : RGB(139, 43, 55));
    solidrectangle(capeX - 9, capeY + 1, capeX + 9, capeY + 11);
    setfillcolor(flashing ? RGB(235, 235, 235) : RGB(190, 57, 61));
    solidrectangle(capeX - 7, capeY + 2, capeX + 5, capeY + 4);

    drawOrientedPixelHammer(centerX, centerY, player.direction);

    // 参考素材的头盔骑士：红色羽饰、银灰头盔、蓝色甲衣和硬边黑描边。
    setfillcolor(RGB(19, 20, 27));
    solidrectangle(centerX - 13, centerY - 12 - breath, centerX + 13, centerY + 12);
    solidrectangle(centerX - 9, centerY - 17 - breath, centerX + 9, centerY + 13);
    setfillcolor(flashing ? WHITE : RGB(45, 103, 190));
    solidrectangle(centerX - 11, centerY + 2, centerX + 11, centerY + 11);
    setfillcolor(flashing ? WHITE : RGB(132, 145, 159));
    solidrectangle(centerX - 10, centerY - 10 - breath, centerX + 10, centerY + 3);
    solidrectangle(centerX - 7, centerY - 14 - breath, centerX + 7, centerY - 9);
    setfillcolor(flashing ? WHITE : RGB(214, 222, 228));
    solidrectangle(centerX - 7, centerY - 12 - breath, centerX + 5, centerY - 10);
    setfillcolor(RGB(34, 35, 42));
    solidrectangle(centerX - 9, centerY - 4 - breath, centerX + 9, centerY + 1);
    setfillcolor(flashing ? WHITE : RGB(230, 71, 73));
    solidrectangle(centerX - 12, centerY - 17 - breath, centerX - 5, centerY - 14 - breath);
    solidrectangle(centerX - 16, centerY - 15 - breath, centerX - 9, centerY - 11 - breath);
    setfillcolor(flashing ? WHITE : RGB(77, 139, 216));
    solidrectangle(centerX - 12, centerY + 2, centerX - 8, centerY + 7);
    solidrectangle(centerX + 8, centerY + 2, centerX + 12, centerY + 7);
    setfillcolor(flashing ? RGB(238, 238, 238) : RGB(224, 184, 77));
    solidrectangle(centerX - 8, centerY + 7, centerX + 8, centerY + 9);
    setfillcolor(RGB(20, 21, 27));
    solidrectangle(centerX - 8, centerY + 11, centerX - 3, centerY + 15);
    solidrectangle(centerX + 3, centerY + 11, centerX + 8, centerY + 15);
}

void GameUI::drawHUD(const Game& game) {
    setfillcolor(RGB(31, 35, 42));
    solidrectangle(MAP_PIXEL_WIDTH, 0, WINDOW_WIDTH, MAP_PIXEL_HEIGHT);
    for (int y = 0; y < MAP_PIXEL_HEIGHT; y += 32) {
        setfillcolor((y / 32) % 2 == 0 ? RGB(43, 47, 55) : RGB(38, 42, 50));
        solidrectangle(MAP_PIXEL_WIDTH + 2, y + 2, WINDOW_WIDTH, y + 30);
        setfillcolor(RGB(24, 28, 35));
        solidrectangle(MAP_PIXEL_WIDTH + 2, y + 30, WINDOW_WIDTH, y + 32);
    }
    setfillcolor(RGB(15, 18, 24));
    solidrectangle(MAP_PIXEL_WIDTH - 3, 0, MAP_PIXEL_WIDTH + 4, MAP_PIXEL_HEIGHT);
    setfillcolor(RGB(102, 110, 123));
    solidrectangle(MAP_PIXEL_WIDTH, 0, MAP_PIXEL_WIDTH + 2, MAP_PIXEL_HEIGHT);

    drawPanel(808, 6, 992, 54, RGB(41, 45, 53), RGB(154, 128, 71), 0);
    drawHammerIcon(834, 22, 0.31);
    useFont(20, RGB(237, 211, 143), FW_BOLD);
    drawCenteredTextShadow(915, 17, L"打个锤子", RGB(237, 211, 143));
    drawVine(986, 10, 35, true);

    drawPanel(810, 62, 990, 126, RGB(38, 43, 52), RGB(99, 107, 121), 0);
    drawHudIcon(HudIcon::Level, 829, 84, RGB(115, 174, 221));
    drawHudIcon(HudIcon::Turn, 918, 84, RGB(224, 189, 92));
    useFont(13, RGB(157, 169, 186), FW_BOLD);
    outtextxy(843, 69, L"关卡");
    outtextxy(932, 69, L"回合");
    useFont(20, RGB(236, 239, 243), FW_BOLD);
    outtextxy(843, 91, (std::to_wstring(game.currentLevel()) + L"/" + std::to_wstring(game.totalLevels())).c_str());
    outtextxy(932, 91, std::to_wstring(game.player().turnCount).c_str());

    drawPanel(810, 134, 990, 226, RGB(38, 43, 52),
              animations_.isPlayerDamageActive() ? RGB(245, 75, 80)
              : (animations_.isPlayerHealActive() ? RGB(75, 225, 120) : RGB(105, 111, 123)), 0);
    drawHudIcon(HudIcon::Heart, 830, 153, RGB(234, 67, 75));
    useFont(15, RGB(190, 198, 210), FW_BOLD);
    outtextxy(847, 143, L"猎人生命");

    if (displayedHp_ < 0.0 || displayedHpLevel_ != game.currentLevel()) {
        displayedHp_ = static_cast<double>(game.player().hp);
        displayedHpLevel_ = game.currentLevel();
    } else {
        displayedHp_ += (static_cast<double>(game.player().hp) - displayedHp_) * 0.18;
        if (std::abs(displayedHp_ - static_cast<double>(game.player().hp)) < 0.03) {
            displayedHp_ = static_cast<double>(game.player().hp);
        }
    }
    const double hpRatio = game.player().maxHp > 0
                               ? std::clamp(displayedHp_ / static_cast<double>(game.player().maxHp), 0.0, 1.0)
                               : 0.0;
    const COLORREF hpColor = hpRatio >= 0.7 ? RGB(75, 210, 112)
                              : (hpRatio >= 0.3 ? RGB(241, 193, 58) : RGB(232, 66, 72));
    const COLORREF hpHighlight = hpRatio >= 0.7 ? RGB(121, 242, 143)
                                  : (hpRatio >= 0.3 ? RGB(255, 222, 91) : RGB(255, 111, 116));
    drawSegmentedBar(824, 174, 976, 194, hpRatio, hpColor, hpHighlight);
    useFont(15, WHITE, FW_BOLD);
    drawCenteredText(900, 199, hpText(game.player()));

    drawPanel(810, 234, 990, 340, RGB(38, 43, 52), RGB(105, 111, 123), 0);
    useFont(14, RGB(173, 182, 196), FW_BOLD);
    outtextxy(824, 244, L"战斗状态");
    drawHudIcon(HudIcon::Hammer, 830, 276, RGB(224, 188, 95));
    drawHudIcon(HudIcon::Push, 918, 276, RGB(106, 177, 224));
    useFont(16, RGB(229, 234, 242), FW_BOLD);
    outtextxy(844, 267, (L"攻击 " + std::to_wstring(game.player().attack)).c_str());
    outtextxy(932, 267, (L"击退 " + std::to_wstring(game.player().pushPower)).c_str());
    drawHudIcon(HudIcon::Monster, 830, 310,
                game.aliveMonsterCount() == 0 ? RGB(231, 199, 101) : RGB(216, 93, 96));
    if (game.aliveMonsterCount() == 0) {
        useFont(16, RGB(244, 207, 91), FW_BOLD);
        outtextxy(848, 301, L"出口已开启");
    } else {
        useFont(16, RGB(230, 137, 139), FW_BOLD);
        outtextxy(848, 301, (L"剩余怪物 " + std::to_wstring(game.aliveMonsterCount())).c_str());
    }
    if (hasRagingBoss(game)) {
        useFont(12, RGB(255, 78, 82), FW_BOLD);
        outtextxy(848, 321, L"警告：守卫狂暴");
    }

    drawPanel(810, 348, 990, 470, RGB(38, 43, 52), RGB(105, 111, 123), 0);
    useFont(14, RGB(173, 182, 196), FW_BOLD);
    outtextxy(824, 358, L"地牢记录");
    drawHudIcon(HudIcon::Skull, 830, 391, RGB(213, 218, 224));
    drawHudIcon(HudIcon::Trap, 830, 423, RGB(224, 126, 92));
    drawHudIcon(HudIcon::Blast, 830, 455, RGB(242, 194, 69));
    useFont(16, RGB(224, 229, 236), FW_BOLD);
    outtextxy(848, 382, (L"击败怪物  " + std::to_wstring(game.player().killCount)).c_str());
    outtextxy(848, 414, (L"机关击杀  " + std::to_wstring(game.player().trapKillCount)).c_str());
    outtextxy(848, 446, (L"引爆次数  " + std::to_wstring(game.player().barrelUsedCount)).c_str());
}

void GameUI::drawLogs(const Game& game) const {
    setfillcolor(RGB(27, 31, 38));
    solidrectangle(0, MAP_PIXEL_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT);
    setfillcolor(RGB(14, 17, 22));
    solidrectangle(0, MAP_PIXEL_HEIGHT, WINDOW_WIDTH, MAP_PIXEL_HEIGHT + 5);
    setfillcolor(RGB(96, 103, 114));
    solidrectangle(0, MAP_PIXEL_HEIGHT + 5, WINDOW_WIDTH, MAP_PIXEL_HEIGHT + 7);

    drawPanel(10, 490, 200, 638, RGB(38, 43, 51), RGB(106, 111, 122), 0);
    drawVine(18, 497, 52);
    drawHammerIcon(43, 515, 0.30);
    useFont(18, RGB(231, 202, 128), FW_BOLD);
    outtextxy(65, 505, L"猎人指令");
    setfillcolor(RGB(70, 76, 87));
    solidrectangle(24, 541, 185, 543);
    useFont(13, RGB(170, 181, 198), FW_BOLD);
    outtextxy(28, 553, L"WASD / 方向键  移动");
    outtextxy(28, 577, L"J / SPACE       挥锤");
    outtextxy(28, 601, L"R               重开");

    const auto& entries = game.logEntries();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const int top = 493 + static_cast<int>(index) * 35;
        const COLORREF color = logColor(entries[index].category);
        setfillcolor(RGB(12, 16, 23));
        solidrectangle(211, top, 990, top + 30);
        setfillcolor(index + 1 == entries.size() ? RGB(48, 53, 61) : RGB(36, 41, 49));
        solidrectangle(214, top + 3, 987, top + 27);
        setfillcolor(color);
        solidrectangle(217, top + 7, 222, top + 23);
        solidrectangle(224, top + 11, 227, top + 19);
        useFont(index + 1 == entries.size() ? 17 : 16, color,
                index + 1 == entries.size() ? FW_BOLD : FW_NORMAL);
        const std::wstring visibleText = fitTextToWidth(entries[index].text, 735);
        outtextxy(236, top + 4, visibleText.c_str());
    }
}

void GameUI::drawLevelClear(const Game& game) {
    drawGame(game);
    drawPanel(235, 176, 765, 410, RGB(29, 34, 45), RGB(247, 205, 76), 18);
    setfillcolor(RGB(67, 160, 171));
    solidrectangle(245, 186, 755, 189);
    solidrectangle(245, 397, 755, 400);
    solidrectangle(245, 186, 248, 400);
    solidrectangle(752, 186, 755, 400);
    useFont(40, RGB(250, 211, 83), FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 222, L"第 " + std::to_wstring(game.currentLevel()) + L" 关通过！");
    useFont(20, RGB(197, 209, 227));
    drawCenteredText(WINDOW_WIDTH / 2, 292, L"机关路线已清理，通往更深处的门打开了。");
    drawPanel(350, 342, 650, 383, RGB(48, 55, 70), RGB(247, 205, 76), 8);
    useFont(18, WHITE, FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 351, L"ENTER  进入下一关");
}

void GameUI::drawUpgrade(const Game& game) const {
    drawBrickBackdrop(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(22, 25, 34));
    useFont(36, RGB(250, 207, 76), FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 24, L"机关猎人 · 能力升级");
    useFont(17, RGB(164, 177, 199));
    drawCenteredText(WINDOW_WIDTH / 2, 72, L"第 " + std::to_wstring(game.currentLevel()) + L" 关完成  ·  按数字 1–5 选择一张能力卡");

    struct Card { int x; int y; const wchar_t* number; const wchar_t* title; const wchar_t* line1; const wchar_t* line2; };
    const std::array<Card, 5> cards{{
        {80, 125, L"1", L"强壮体魄", L"MaxHP +4", L"并恢复 4 HP"},
        {370, 125, L"2", L"重锤训练", L"撞墙伤害 +1", L"可重复选择"},
        {660, 125, L"3", L"强力击退", L"击退距离 +1", L"最多一次"},
        {225, 340, L"4", L"尖刺熟练", L"尖刺伤害 +1", L"可重复选择"},
        {515, 340, L"5", L"爆破专家", L"炸药桶 +2", L"可重复选择"}
    }};
    const int pulse = static_cast<int>((frameCount_ / 15U) % 3U);
    for (std::size_t index = 0; index < cards.size(); ++index) {
        const Card& card = cards[index];
        const bool disabled = index == 2 && game.player().strongPushUpgraded;
        drawPanel(card.x, card.y, card.x + 260, card.y + 165,
                  disabled ? RGB(48, 38, 43) : RGB(36, 42, 55),
                  disabled ? RGB(132, 65, 70) : dynamicRgb(78 + pulse * 5, 111 + pulse * 4, 151 + pulse * 5), 14);
        const std::array<HudIcon, 5> icons{{HudIcon::Heart, HudIcon::Hammer, HudIcon::Push,
                                           HudIcon::Trap, HudIcon::Blast}};
        const std::array<COLORREF, 5> iconColors{{RGB(230, 79, 84), RGB(228, 191, 98),
                                                  RGB(104, 178, 224), RGB(225, 125, 89),
                                                  RGB(245, 195, 65)}};
        drawHudIcon(icons[index], card.x + 31, card.y + 31,
                    disabled ? RGB(128, 81, 86) : iconColors[index]);
        useFont(13, disabled ? RGB(157, 116, 120) : RGB(243, 216, 137), FW_BOLD);
        outtextxy(card.x + 47, card.y + 37, card.number);
        useFont(22, disabled ? RGB(185, 120, 125) : RGB(244, 211, 102), FW_BOLD);
        outtextxy(card.x + 60, card.y + 17, card.title);
        setlinecolor(RGB(71, 82, 105));
        line(card.x + 18, card.y + 62, card.x + 242, card.y + 62);
        useFont(18, disabled ? RGB(155, 126, 130) : RGB(225, 232, 242), FW_BOLD);
        drawCenteredText(card.x + 130, card.y + 82, disabled ? L"已达上限" : card.line1);
        useFont(16, RGB(151, 165, 189));
        drawCenteredText(card.x + 130, card.y + 119, disabled ? L"请选择其他能力" : card.line2);
    }
    useFont(17, RGB(168, 180, 201));
    drawCenteredText(WINDOW_WIDTH / 2, 578, L"ESC  返回主菜单");
}

void GameUI::drawGameOver(const Game& game) const {
    drawBrickBackdrop(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(33, 22, 28));
    const int pulse = static_cast<int>((frameCount_ / 12U) % 3U);
    setfillcolor(dynamicRgb(82 + pulse * 12, 31, 43));
    for (int radius = 70; radius <= 230; radius += 40) {
        solidrectangle(WINDOW_WIDTH / 2 - radius, 210 - radius,
                       WINDOW_WIDTH / 2 + radius, 212 - radius);
        solidrectangle(WINDOW_WIDTH / 2 - radius, 208 + radius,
                       WINDOW_WIDTH / 2 + radius, 210 + radius);
    }
    useFont(50, RGB(239, 69, 76), FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 54, L"你倒下了……");
    useFont(20, RGB(205, 190, 198));
    drawCenteredText(WINDOW_WIDTH / 2, 122, L"地牢会记住这次尝试，但机关猎人还能再来一次。");

    drawPanel(290, 182, 710, 472, RGB(48, 34, 42), RGB(128, 63, 75), 16);
    useFont(21, RGB(238, 225, 230), FW_BOLD);
    const std::array<std::wstring, 5> lines{{
        L"当前关卡        " + std::to_wstring(game.currentLevel()),
        L"总回合数        " + std::to_wstring(game.player().turnCount),
        L"击败怪物        " + std::to_wstring(game.player().killCount),
        L"陷阱击杀        " + std::to_wstring(game.player().trapKillCount),
        L"触发爆炸        " + std::to_wstring(game.player().barrelUsedCount)
    }};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        outtextxy(345, 216 + static_cast<int>(i) * 48, lines[i].c_str());
    }
    drawPanel(286, 514, 498, 560, RGB(81, 43, 50), RGB(228, 75, 81), 9);
    drawPanel(512, 514, 724, 560, RGB(43, 48, 61), RGB(95, 113, 143), 9);
    useFont(18, WHITE, FW_BOLD);
    drawCenteredText(392, 525, L"R  重新开始");
    drawCenteredText(618, 525, L"ESC  返回菜单");
}

void GameUI::drawGameWin(const Game& game) const {
    drawBrickBackdrop(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(22, 31, 35));
    const int pulse = static_cast<int>((frameCount_ / 10U) % 5U);
    setfillcolor(dynamicRgb(51, 132 + pulse * 7, 143 + pulse * 7));
    for (int radius = 80; radius <= 250; radius += 42) {
        const int size = radius + pulse;
        solidrectangle(WINDOW_WIDTH / 2 - size, 205 - size,
                       WINDOW_WIDTH / 2 + size, 207 - size);
        solidrectangle(WINDOW_WIDTH / 2 - size, 203 + size,
                       WINDOW_WIDTH / 2 + size, 205 + size);
    }
    useFont(50, RGB(251, 213, 84), FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 45, L"通关成功！");
    useFont(21, RGB(207, 224, 226));
    drawCenteredText(WINDOW_WIDTH / 2, 113, L"你让古老机关成为最强的武器，机关守卫已经倒下。");

    drawPanel(278, 175, 722, 478, RGB(33, 47, 52), RGB(86, 164, 169), 16);
    drawHammerIcon(WINDOW_WIDTH / 2, 211, 0.75);
    useFont(20, RGB(234, 240, 241), FW_BOLD);
    const std::array<std::wstring, 5> lines{{
        L"总回合数        " + std::to_wstring(game.player().turnCount),
        L"击败怪物        " + std::to_wstring(game.player().killCount),
        L"陷阱击杀        " + std::to_wstring(game.player().trapKillCount),
        L"触发爆炸        " + std::to_wstring(game.player().barrelUsedCount),
        L"剩余生命        " + hpText(game.player())
    }};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        outtextxy(340, 272 + static_cast<int>(i) * 42, lines[i].c_str());
    }
    drawPanel(365, 526, 635, 571, RGB(51, 68, 71), RGB(245, 207, 79), 9);
    useFont(18, WHITE, FW_BOLD);
    drawCenteredText(WINDOW_WIDTH / 2, 537, L"ESC  返回主菜单");
}

InputKey GameUI::getInput() const {
    if (!initialized_) {
        return InputKey::None;
    }
    const HWND window = GetHWnd();
    if (window == nullptr || !IsWindow(window)) {
        return InputKey::Quit;
    }

    ExMessage message{};
    while (peekmessage(&message, EX_KEY | EX_WINDOW)) {
        if (message.message == WM_CLOSE) {
            return InputKey::Quit;
        }
        if (message.message != WM_KEYDOWN || message.prevdown) {
            continue;
        }
        switch (message.vkcode) {
        case 'W':
        case VK_UP:
            return InputKey::MoveUp;
        case 'S':
        case VK_DOWN:
            return InputKey::MoveDown;
        case 'A':
        case VK_LEFT:
            return InputKey::MoveLeft;
        case 'D':
        case VK_RIGHT:
            return InputKey::MoveRight;
        case 'J':
        case VK_SPACE:
            return InputKey::Attack;
        case 'R':
            return InputKey::Restart;
        case 'H':
            return InputKey::Help;
        case VK_RETURN:
            return InputKey::Confirm;
        case VK_ESCAPE:
            return InputKey::Escape;
        case '1':
        case VK_NUMPAD1:
            return InputKey::Upgrade1;
        case '2':
        case VK_NUMPAD2:
            return InputKey::Upgrade2;
        case '3':
        case VK_NUMPAD3:
            return InputKey::Upgrade3;
        case '4':
        case VK_NUMPAD4:
            return InputKey::Upgrade4;
        case '5':
        case VK_NUMPAD5:
            return InputKey::Upgrade5;
        default:
            break;
        }
    }
    return InputKey::None;
}

} // namespace dungeon
