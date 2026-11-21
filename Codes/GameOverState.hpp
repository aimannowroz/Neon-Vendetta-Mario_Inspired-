#pragma once
#include "GameState.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "SoundManager.hpp"

//  GameOverState — death/timeout screen. Player can retry(enter key) or quit(esc to menu).
class GameOverState : public GameState {
    Texture& bgTex;
    Sprite   bg;
    RectangleShape overlay;

    Font& font;
    Text title, hint, retryHint, menuHint;
    Clock blink;
    bool show = true;

public:
    bool retryRequested = false;
    bool menuRequested  = false;

    GameOverState()
        : bgTex(AssetCache::get().tex("../assets/levels/industrial/bg/Background.png")),
          bg(bgTex),
          font(AssetCache::get().font("../assets/CyberpunkCraftpixPixel.otf")),
          title(font), hint(font), retryHint(font), menuHint(font)
    {
        Vector2u s = bgTex.getSize();
        bg.setScale({G::SCREEN_W / s.x, G::SCREEN_H / s.y});
        overlay.setSize({G::SCREEN_W, G::SCREEN_H});
        overlay.setFillColor(Color(40, 0, 0, 200));     // red-tinted overlay

        title.setString("GAME  OVER");
        title.setCharacterSize(140);
        title.setFillColor(Color(255, 60, 60));
        title.setOutlineColor(Color::Black);
        title.setOutlineThickness(6.f);
        FloatRect b = title.getLocalBounds();
        title.setOrigin({b.size.x / 2.f, 0.f});
        title.setPosition({G::SCREEN_W / 2.f, 280.f});

        hint.setString("Your run ended.");
        hint.setCharacterSize(40);
        hint.setFillColor(Color(220, 180, 180));
        b = hint.getLocalBounds();
        hint.setOrigin({b.size.x / 2.f, 0.f});
        hint.setPosition({G::SCREEN_W / 2.f, 530.f});

        retryHint.setString("Press  ENTER  to  Retry");
        retryHint.setCharacterSize(54);
        retryHint.setFillColor(Color::White);
        retryHint.setOutlineColor(Color::Black);
        retryHint.setOutlineThickness(3.f);
        b = retryHint.getLocalBounds();
        retryHint.setOrigin({b.size.x / 2.f, 0.f});
        retryHint.setPosition({G::SCREEN_W / 2.f, 650.f});

        menuHint.setString("Press  ESC  for  Main  Menu");
        menuHint.setCharacterSize(36);
        menuHint.setFillColor(Color(180, 180, 180));
        b = menuHint.getLocalBounds();
        menuHint.setOrigin({b.size.x / 2.f, 0.f});
        menuHint.setPosition({G::SCREEN_W / 2.f, 750.f});
    }

    void onEnter() override {
        SoundManager::get().stopMusic();
        retryRequested = menuRequested = false;
    }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Enter)  retryRequested = true;
            if (k->code == Keyboard::Key::Escape) menuRequested  = true;
        }
    }

    void update(float dt) override {
        if (blink.getElapsedTime().asSeconds() > 0.5f) {
            show = !show;
            blink.restart();
        }
    }

    void render(RenderWindow& w) override {
        w.setView(w.getDefaultView());
        w.clear(Color::Black);
        w.draw(bg);
        w.draw(overlay);
        w.draw(title);
        w.draw(hint);
        if (show) w.draw(retryHint);
        w.draw(menuHint);
        w.display();
    }
};
