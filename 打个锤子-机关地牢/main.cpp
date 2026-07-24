#include "core/Game.h"
#include "ui/GameUI.h"

#include <graphics.h>

int main() {
    dungeon::Game game;
    dungeon::GameUI ui;
    ui.initWindow();

    while (game.state() != dungeon::GameState::Exit) {
        const dungeon::InputKey key = ui.getInput();
        if (key != dungeon::InputKey::None) {
            game.handleInput(key);
        }
        if (game.state() == dungeon::GameState::Exit) {
            break;
        }
        ui.consumeEvents(game.takeVisualEvents());
        ui.draw(game);
        Sleep(16);
    }

    ui.closeWindow();
    return 0;
}
