//  State  flow:
//     Menu  ─► CharSelect  ─► Level1  ┬─► Win(Continue) ─► Level2 ┬─► Win(Victory) ─► Menu
//                                      └─► GameOver ──► (retry)    └─► GameOver
//                  Any level can be paused with ESC, resumed with SPACE.

#include "libraries.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "SoundManager.hpp"
#include "MenuState.hpp"
#include "CharSelectState.hpp"
#include "LevelState.hpp"
#include "PauseState.hpp"
#include "WinState.hpp"
#include "GameOverState.hpp"

int main() {
    // Window setup 
    RenderWindow window(VideoMode({(unsigned)G::SCREEN_W, (unsigned)G::SCREEN_H}),
                        "Neon Vendetta",
                        State::Fullscreen);
    window.setFramerateLimit(60);

    // Force-construct the SoundManager up front so first-frame
    // doesn't stutter on music load.
    SoundManager::get();

    // All screens 
    MenuState       menu;
    CharSelectState charSelect;
    LevelState      levelScreen;
    PauseState      pauseScreen;
    WinState        winScreen;
    GameOverState   overScreen;

    GameState* current = &menu;
    current->onEnter();

    Clock deltaClock;

    while (window.isOpen()) {
        float dt = min(deltaClock.restart().asSeconds(), G::MAX_DT);

        // Events
        while (auto ev = window.pollEvent())
            current->handleEvents(ev, window);

        // State transitions
        auto switchTo = [&](GameState* next) {
            current->onExit();
            current = next;
            current->onEnter();
        };

        // Menu → CharSelect
        if (current == &menu && menu.startSelected) {
            menu.startSelected = false;
            switchTo(&charSelect);
        }
        // CharSelect → LevelState (level 1)
        else if (current == &charSelect && charSelect.confirmed) {
            charSelect.confirmed = false;
            levelScreen.resetCarry();
            levelScreen.setupForCharacter(charSelect.chosenCharDir);
            levelScreen.startLevel(1);
            switchTo(&levelScreen);
        }
        // Level → Pause
        else if (current == &levelScreen && levelScreen.requestPause) {
            levelScreen.requestPause = false;
            switchTo(&pauseScreen);
        }
        // Pause → Level (resume)
        else if (current == &pauseScreen && pauseScreen.resumeRequested) {
            pauseScreen.resumeRequested = false;
            switchTo(&levelScreen);
        }
        // Pause → Menu (quit)
        else if (current == &pauseScreen && pauseScreen.quitRequested) {
            pauseScreen.quitRequested = false;
            SoundManager::get().stopMusic();
            menu.refreshBest();
            switchTo(&menu);
        }
        // Level → Win (between-level transition)
        else if (current == &levelScreen && levelScreen.requestNextLevel) {
            levelScreen.requestNextLevel = false;
            winScreen.setRunResult(WinState::Mode::Continue,
                                    levelScreen.finalScore(),
                                    levelScreen.finalCoins());
            switchTo(&winScreen);
        }
        // Level → Win (final victory)
        else if (current == &levelScreen && levelScreen.requestComplete) {
            levelScreen.requestComplete = false;
            winScreen.setRunResult(WinState::Mode::Victory,
                                    levelScreen.finalScore(),
                                    levelScreen.finalCoins());
            switchTo(&winScreen);
        }
        // Level → GameOver
        else if (current == &levelScreen && levelScreen.requestGameOver) {
            levelScreen.requestGameOver = false;
            switchTo(&overScreen);
        }
        // Win → Level2 (after level 1)
        else if (current == &winScreen && winScreen.advanceRequested) {
            winScreen.advanceRequested = false;
            levelScreen.startLevel(2);
            switchTo(&levelScreen);
        }
        // Win → Menu (after final victory)
        else if (current == &winScreen && winScreen.menuRequested) {
            winScreen.menuRequested = false;
            menu.refreshBest();
            switchTo(&menu);
        }
        // GameOver → retry from level 1 with same character
        else if (current == &overScreen && overScreen.retryRequested) {
            overScreen.retryRequested = false;
            levelScreen.resetCarry();
            levelScreen.startLevel(1);
            switchTo(&levelScreen);
        }
        // GameOver → Menu
        else if (current == &overScreen && overScreen.menuRequested) {
            overScreen.menuRequested = false;
            menu.refreshBest();
            switchTo(&menu);
        }
        //  Update + Render 
        current->update(dt);
        current->render(window);
    }
    return 0;
}
