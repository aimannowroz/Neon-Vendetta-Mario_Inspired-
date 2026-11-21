#pragma once
#include "GameState.hpp"
#include "Player.hpp"
#include "Level.hpp"
#include "SoundManager.hpp"

// ════════════════════════════════════════════════════════════════
//  LevelState — gameplay screen.
//  OWNS the Player and the current Level.
// ════════════════════════════════════════════════════════════════
class LevelState : public GameState {
    optional<Player> player;
    optional<Level>  level;
    string charDir;
    int  levelNumber = 1;        // 1 = Industrial, 2 = Green
    int  carryScore  = 0;        // score brought from previous level

public:
    bool requestPause      = false;
    bool requestNextLevel  = false;
    bool requestGameOver   = false;
    bool requestComplete   = false;   // beat the entire game

    void setupForCharacter(const string& dir) {
        charDir = dir;
    }

    void startLevel(int n) {
        levelNumber = n;
        // Re-create Player & Level from scratch
        player.emplace(charDir);
        level.emplace();
        level->setPlayer(&*player);
        level->initHUD();
        if (n == 1) level->setupIndustrial();
        else        level->setupGreen();

        level->score = carryScore;     // accumulate across levels

        requestPause = requestNextLevel = requestGameOver = requestComplete = false;

        SoundManager::get().startMusic();
    }

    void onEnter() override {
        // Resume music if returning from pause
        SoundManager::get().startMusic();
    }

    int currentLevel() const { return levelNumber; }
    int currentScore() const { return level ? level->totalScore() : 0; }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Escape) requestPause = true;
            if (k->code == Keyboard::Key::W)      level->onJumpPressed();
            if (k->code == Keyboard::Key::Up)     level->onJumpPressed();
            if (k->code == Keyboard::Key::J)      level->onAttackPressed();
        }
    }

    void update(float dt) override {
        if (!level) return;
        level->update(dt);
        if (level->lost) {
            requestGameOver = true;
        } else if (level->won) {
            // Carry score forward
            carryScore = level->totalScore();
            if (levelNumber == 1) requestNextLevel = true;
            else                   requestComplete = true;
        }
    }

    void render(RenderWindow& w) override {
        if (level) level->render(w);
        w.display();
    }

    int  finalScore()  const { return level ? level->totalScore() : 0; }
    int  finalCoins()  const { return level ? level->coins() : 0; }
    void resetCarry()        { carryScore = 0; }
};
