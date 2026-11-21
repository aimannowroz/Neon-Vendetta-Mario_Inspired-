#pragma once
#include "GameState.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "BestScore.hpp"

//  MenuState — title screen.
class MenuState : public GameState {
    Texture& bgTex;
    Sprite   bg;
    RectangleShape overlay;

    // Decorative frame around the title block
    RectangleShape topRule, bottomRule;

    Font& font;
    Text titleMain;        // foreground white + outline
    Text titleShadowCyan;  // cyan offset shadow
    Text titleShadowMag;   // magenta offset shadow
    Text subtitle;         // small accent line above title
    Text playPrompt;
    Text controlsHint;
    Text bestText;
    Text creditText;

    Clock blinkClock;
    bool  promptVisible = true;

public:
    bool startSelected = false;

    MenuState()
        : bgTex(AssetCache::get().tex("../assets/levels/industrial/bg/Background.png")),
          bg(bgTex),
          font(AssetCache::get().font("../assets/CyberpunkCraftpixPixel.otf")),
          titleMain(font),
          titleShadowCyan(font),
          titleShadowMag(font),
          subtitle(font),
          playPrompt(font),
          controlsHint(font),
          bestText(font),
          creditText(font)
    {
        // Background fits screen, dimmed 
        Vector2u s = bgTex.getSize();
        bg.setScale({G::SCREEN_W / s.x, G::SCREEN_H / s.y});

        overlay.setSize({G::SCREEN_W, G::SCREEN_H});
        overlay.setFillColor(Color(0, 0, 0, 170));

        // Decorative horizontal rules above & below title 
        topRule.setSize({780.f, 4.f});
        topRule.setFillColor(Color(0, 230, 255));
        topRule.setOrigin({390.f, 2.f});
        topRule.setPosition({G::SCREEN_W / 2.f, 210.f});

        bottomRule.setSize({780.f, 4.f});
        bottomRule.setFillColor(Color(255, 80, 160));
        bottomRule.setOrigin({390.f, 2.f});
        bottomRule.setPosition({G::SCREEN_W / 2.f, 430.f});

        // Subtitle marquee above title 
        subtitle.setCharacterSize(32);
        subtitle.setFillColor(Color(255, 80, 160, 230));
        subtitle.setOutlineColor(Color::Black);
        subtitle.setOutlineThickness(2.f);
        FloatRect sb = subtitle.getLocalBounds();
        subtitle.setOrigin({sb.size.x / 2.f, 0.f});
        subtitle.setPosition({G::SCREEN_W / 2.f, 220.f});

        //  Title (3 chromatic layers)
        const string TITLE = "NEON  VENDETTA";

        titleShadowCyan.setString(TITLE);
        titleShadowCyan.setCharacterSize(140);
        titleShadowCyan.setFillColor(Color(0, 230, 255, 220));
        FloatRect tb = titleShadowCyan.getLocalBounds();
        titleShadowCyan.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});

        titleShadowMag.setString(TITLE);
        titleShadowMag.setCharacterSize(140);
        titleShadowMag.setFillColor(Color(255, 40, 140, 220));
        titleShadowMag.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});

        titleMain.setString(TITLE);
        titleMain.setCharacterSize(140);
        titleMain.setFillColor(Color::White);
        titleMain.setOutlineColor(Color::Black);
        titleMain.setOutlineThickness(4.f);
        titleMain.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});

        Vector2f titlePos{G::SCREEN_W / 2.f, 320.f};
        titleShadowMag .setPosition({titlePos.x - 5.f, titlePos.y + 4.f});
        titleShadowCyan.setPosition({titlePos.x + 5.f, titlePos.y - 4.f});
        titleMain      .setPosition(titlePos);

        //  Play prompt
        playPrompt.setString("[  PRESS  ENTER  TO  PLAY  ]");
        playPrompt.setCharacterSize(56);
        playPrompt.setFillColor(Color::White);
        playPrompt.setOutlineColor(Color::Black);
        playPrompt.setOutlineThickness(3.f);
        FloatRect pb = playPrompt.getLocalBounds();
        playPrompt.setOrigin({pb.size.x / 2.f, 0.f});
        playPrompt.setPosition({G::SCREEN_W / 2.f, 620.f});

        //  Controls hint
        controlsHint.setString(
            "A/D : MOVE        W : JUMP  (W AGAIN = DOUBLE JUMP)        "
            "J : ATTACK        ESC : PAUSE");
        controlsHint.setCharacterSize(26);
        controlsHint.setFillColor(Color(200, 200, 200));
        controlsHint.setOutlineColor(Color::Black);
        controlsHint.setOutlineThickness(1.f);
        FloatRect cb = controlsHint.getLocalBounds();
        controlsHint.setOrigin({cb.size.x / 2.f, 0.f});
        controlsHint.setPosition({G::SCREEN_W / 2.f, 800.f});

        //  Best score 
        bestText.setCharacterSize(30);
        bestText.setOutlineColor(Color::Black);
        bestText.setOutlineThickness(2.f);
        refreshBest();

        //  Credit 
        creditText.setString("ASSETS  BY  CRAFTPIX.NET     //     BUILT  WITH  SFML 3");
        creditText.setCharacterSize(20);
        creditText.setFillColor(Color(140, 140, 150));
        FloatRect rb = creditText.getLocalBounds();
        creditText.setOrigin({rb.size.x / 2.f, 0.f});
        creditText.setPosition({G::SCREEN_W / 2.f, 1030.f});
    }

    void refreshBest() {
        int b = BestScore::load();
        bestText.setString("BEST  SCORE :   " + to_string(b));
        bestText.setFillColor(b > 0 ? Color(0, 230, 255) : Color(140, 140, 140));
        FloatRect bb = bestText.getLocalBounds();
        bestText.setOrigin({bb.size.x / 2.f, 0.f});
        bestText.setPosition({G::SCREEN_W / 2.f, 880.f});
    }

    void onEnter() override {
        startSelected = false;
    }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Enter)  startSelected = true;
            if (k->code == Keyboard::Key::Escape) window.close();
        }
    }

    void update(float dt) override {
        if (blinkClock.getElapsedTime().asSeconds() > 0.55f) {
            promptVisible = !promptVisible;
            blinkClock.restart();
        }
    }

    void render(RenderWindow& w) override {
        w.setView(w.getDefaultView());
        w.clear(Color::Black);

        w.draw(bg);
        w.draw(overlay);

        w.draw(topRule);
        w.draw(titleShadowMag);
        w.draw(titleShadowCyan);
        w.draw(titleMain);
        w.draw(bottomRule);

        if (promptVisible) w.draw(playPrompt);
        w.draw(controlsHint);
        w.draw(bestText);
        w.draw(creditText);

        w.display();
    }
};