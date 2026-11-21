#pragma once
#include "GameState.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "BestScore.hpp"
#include "SoundManager.hpp"

//  WinState is used both as between-level transition and as final
//  victory screen.  setMode() chooses which.
//    Mode::Continue  —  "LEVEL CLEARED" + "ENTER for next level"
//    Mode::Victory   —  "VICTORY" + "ENTER returns to menu"

class WinState : public GameState {
public:
    enum class Mode { Continue, Victory };

private:
    Texture& bgTex;
    Sprite   bg;
    RectangleShape overlay;

    Font& font;
    Text title;
    Text scoreText;
    Text coinsText;
    Text bestText;
    Text recordBanner;
    Text prompt;

    Clock blinkClock, recordPulseClock;
    bool  promptVisible = true;

    Mode  mode = Mode::Continue;
    int   shownScore = 0;
    int   shownCoins = 0;
    bool  newRecord  = false;

public:
    bool advanceRequested = false;     // set when ENTER pressed
    bool menuRequested    = false;     // set on victory ENTER

    //constructor
    WinState()
        : bgTex(AssetCache::get().tex("../assets/levels/industrial/bg/Background.png")),
          bg(bgTex),
          font(AssetCache::get().font("../assets/CyberpunkCraftpixPixel.otf")),
          title(font), scoreText(font), coinsText(font),
          bestText(font), recordBanner(font), prompt(font)
    {
        Vector2u s = bgTex.getSize();
        bg.setScale({G::SCREEN_W / s.x, G::SCREEN_H / s.y});

        overlay.setSize({G::SCREEN_W, G::SCREEN_H});
        overlay.setFillColor(Color(0, 0, 0, 200));

        title.setCharacterSize(110);
        title.setOutlineColor(Color::Black);
        title.setOutlineThickness(5.f);

        scoreText.setCharacterSize(56);
        scoreText.setFillColor(Color::White);
        scoreText.setOutlineColor(Color::Black);
        scoreText.setOutlineThickness(3.f);

        coinsText.setCharacterSize(40);
        coinsText.setFillColor(Color(0, 230, 255));
        coinsText.setOutlineColor(Color::Black);
        coinsText.setOutlineThickness(2.f);

        bestText.setCharacterSize(34);
        bestText.setFillColor(Color(180, 180, 180));

        recordBanner.setString(">>>   NEW   RECORD   <<<");
        recordBanner.setCharacterSize(60);
        recordBanner.setFillColor(Color::Yellow);
        recordBanner.setOutlineColor(Color::Black);
        recordBanner.setOutlineThickness(4.f);

        prompt.setCharacterSize(44);
        prompt.setFillColor(Color::White);
        prompt.setOutlineColor(Color::Black);
        prompt.setOutlineThickness(3.f);
    }

    // Caller sets the run result before activating this state.
    void setRunResult(Mode m, int score, int coins) {
        mode = m;
        shownScore = score;
        shownCoins = coins;
        newRecord = BestScore::maybeSave(score);
        recordPulseClock.restart();

        // Title
        title.setString(m == Mode::Victory ? "VICTORY" : "LEVEL  CLEARED");
        title.setFillColor(m == Mode::Victory ? Color(255, 200, 0)
                                              : Color(0, 230, 255));
        FloatRect b = title.getLocalBounds();
        title.setOrigin({b.size.x / 2.f, 0.f});
        title.setPosition({G::SCREEN_W / 2.f, 200.f});

        // Score
        scoreText.setString("SCORE :   " + to_string(score));
        b = scoreText.getLocalBounds();
        scoreText.setOrigin({b.size.x / 2.f, 0.f});
        scoreText.setPosition({G::SCREEN_W / 2.f, 470.f});

        // Coins
        coinsText.setString("COINS  COLLECTED :   " + to_string(coins));
        b = coinsText.getLocalBounds();
        coinsText.setOrigin({b.size.x / 2.f, 0.f});
        coinsText.setPosition({G::SCREEN_W / 2.f, 560.f});

        // Best
        bestText.setString("BEST :   " + to_string(BestScore::load()));
        b = bestText.getLocalBounds();
        bestText.setOrigin({b.size.x / 2.f, 0.f});
        bestText.setPosition({G::SCREEN_W / 2.f, 640.f});

        // Record banner
        b = recordBanner.getLocalBounds();
        recordBanner.setOrigin({b.size.x / 2.f, 0.f});
        recordBanner.setPosition({G::SCREEN_W / 2.f, 720.f});

        // Prompt
        prompt.setString(m == Mode::Victory
            ? "Press  ENTER  to  return  to  Menu"
            : "Press  ENTER  for  next  level");
        b = prompt.getLocalBounds();
        prompt.setOrigin({b.size.x / 2.f, 0.f});
        prompt.setPosition({G::SCREEN_W / 2.f, 870.f});

        advanceRequested = false;
        menuRequested    = false;
    }

    void onEnter() override {
        SoundManager::get().pauseMusic();
        advanceRequested = false;
        menuRequested    = false;
    }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Enter) {
                if (mode == Mode::Victory) menuRequested    = true;
                else                        advanceRequested = true;
            }
            if (k->code == Keyboard::Key::Escape) menuRequested = true;
        }
    }

    void update(float dt) override {
        if (blinkClock.getElapsedTime().asSeconds() > 0.55f) {
            promptVisible = !promptVisible;
            blinkClock.restart();
        }
        if (newRecord) {
            // Pulse yellow ↔ orange
            float t = (sin(recordPulseClock.getElapsedTime().asSeconds() * 4.f) + 1.f) / 2.f;
            uint8_t g = (uint8_t)(160 + t * 95);
            recordBanner.setFillColor(Color(255, g, 0));
        }
    }
    void render(RenderWindow& w) override {
        w.setView(w.getDefaultView());
        w.clear(Color::Black);
        w.draw(bg);
        w.draw(overlay);
        w.draw(title);
        w.draw(scoreText);
        w.draw(coinsText);
        w.draw(bestText);
        if (newRecord) w.draw(recordBanner);
        if (promptVisible) w.draw(prompt);
        w.display();
    }
};
