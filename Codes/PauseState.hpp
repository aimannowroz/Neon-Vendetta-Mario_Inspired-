#pragma once
#include "GameState.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "SoundManager.hpp"

class PauseState : public GameState {
    Font& font;
    RectangleShape overlay;
    Text title, resume, quit;
    Clock blink;
    bool show = true;

public:
    bool resumeRequested = false;
    bool quitRequested   = false;

    PauseState()
        : font(AssetCache::get().font("../assets/CyberpunkCraftpixPixel.otf")),
          title(font), resume(font), quit(font)
    {
        overlay.setSize({G::SCREEN_W, G::SCREEN_H});
        overlay.setFillColor(Color(0, 0, 0, 200));

        title.setString("PAUSED");
        title.setCharacterSize(120);
        title.setFillColor(Color(0, 230, 255));
        title.setOutlineColor(Color::Black);
        title.setOutlineThickness(5.f);
        FloatRect b = title.getLocalBounds();
        title.setOrigin({b.size.x / 2.f, 0.f});
        title.setPosition({G::SCREEN_W / 2.f, 280.f});

        resume.setString("Press  SPACE  to  Resume");
        resume.setCharacterSize(54);
        resume.setFillColor(Color::White);
        resume.setOutlineColor(Color::Black);
        resume.setOutlineThickness(3.f);
        b = resume.getLocalBounds();
        resume.setOrigin({b.size.x / 2.f, 0.f});
        resume.setPosition({G::SCREEN_W / 2.f, 540.f});

        quit.setString("Press  Q  to  Quit  to  Menu");
        quit.setCharacterSize(40);
        quit.setFillColor(Color(180, 180, 180));
        b = quit.getLocalBounds();
        quit.setOrigin({b.size.x / 2.f, 0.f});
        quit.setPosition({G::SCREEN_W / 2.f, 640.f});
    }

    void onEnter() override {
        SoundManager::get().pauseMusic();
        resumeRequested = quitRequested = false;
    }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Space) resumeRequested = true;
            if (k->code == Keyboard::Key::Escape) resumeRequested = true;
            if (k->code == Keyboard::Key::Q)     quitRequested   = true;
        }
    }

    void update(float dt) override {
        if (blink.getElapsedTime().asSeconds() > 0.5f) {
            show = !show;
            blink.restart();
        }
    }

    void render(RenderWindow& w) override {
        // don't clear  game frame is still underneath
        w.setView(w.getDefaultView());
        w.draw(overlay);
        w.draw(title);
        if (show) w.draw(resume);
        w.draw(quit);
        w.display();
    }
};
