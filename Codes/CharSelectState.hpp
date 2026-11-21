#pragma once
#include "GameState.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "AnimatedSprite.hpp"

//  CharSelectState — pick one of three characters before playing.
class CharSelectState : public GameState {
    Texture& bgTex;
    Sprite   bg;
    RectangleShape overlay;

    Font& font;
    Text title;
    Text controlHint;
    Text labels[3];

    vector<AnimatedSprite> previews;
    vector<string>         names = {"BIKER", "PUNK", "CYBORG"};
    vector<string>         dirs  = {
        "../assets/characters/biker",
        "../assets/characters/punk",
        "../assets/characters/cyborg"
    };

    int  selected = 0;
    bool keyAlreadyDown = false;   // edge-detect key presses

public:
    bool   confirmed     = false;
    string chosenCharDir;
//constructor sets up all the text and animated preview sprites
    CharSelectState()
        : bgTex(AssetCache::get().tex("../assets/levels/industrial/bg/Background.png")),
          bg(bgTex),
          font(AssetCache::get().font("../assets/CyberpunkCraftpixPixel.otf")),
          title(font),
          controlHint(font),
          labels{Text(font), Text(font), Text(font)}
    {
        Vector2u s = bgTex.getSize();
        bg.setScale({G::SCREEN_W / s.x, G::SCREEN_H / s.y});
        overlay.setSize({G::SCREEN_W, G::SCREEN_H});
        overlay.setFillColor(Color(0, 0, 0, 180));

        title.setString("CHOOSE  YOUR  CHARACTER");
        title.setCharacterSize(72);
        title.setFillColor(Color(0, 230, 255));
        title.setOutlineColor(Color::Black);
        title.setOutlineThickness(4.f);
        FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.size.x / 2.f, 0.f});
        title.setPosition({G::SCREEN_W / 2.f, 100.f});

        controlHint.setString("A / D :  Switch        ENTER :  Confirm        ESC :  Back");
        controlHint.setCharacterSize(32);
        controlHint.setFillColor(Color(180, 180, 180));
        FloatRect cb = controlHint.getLocalBounds();
        controlHint.setOrigin({cb.size.x / 2.f, 0.f});
        controlHint.setPosition({G::SCREEN_W / 2.f, 920.f});

        // Build animated previews for each character (idle anim, 4 frames)
        AssetCache& A = AssetCache::get();
        const char* prefixes[3] = {"Biker", "Punk", "Cyborg"};
        for (int i = 0; i < 3; i++) {
            string idlePath = dirs[i] + "/" + prefixes[i] + "_idle.png";
            previews.emplace_back(A.tex(idlePath), 4, 0.18f);
            previews[i].setScale(7.0f);

            labels[i].setString(names[i]);
            labels[i].setCharacterSize(48);
            labels[i].setFillColor(Color::White);
            labels[i].setOutlineColor(Color::Black);
            labels[i].setOutlineThickness(3.f);
            FloatRect lb = labels[i].getLocalBounds();
            labels[i].setOrigin({lb.size.x / 2.f, 0.f});
            labels[i].setPosition({G::SCREEN_W / 4.f * (i + 1), 720.f});

            // Position the preview above its label
            previews[i].setPosition({G::SCREEN_W / 4.f * (i + 1), 600.f});
        }
    }

    void onEnter() override {
        confirmed = false;
        keyAlreadyDown = false;
    }

    void handleEvents(const optional<Event>& ev, RenderWindow& window) override {
        if (ev->is<Event::Closed>()) window.close();
        if (auto k = ev->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Left || k->code == Keyboard::Key::A)
                selected = (selected + 2) % 3;
            if (k->code == Keyboard::Key::Right || k->code == Keyboard::Key::D)
                selected = (selected + 1) % 3;
            if (k->code == Keyboard::Key::Enter) {
                chosenCharDir = dirs[selected];
                confirmed = true;
            }
        }
    }

    void update(float dt) override {
        for (auto& p : previews) p.update(dt);
    }

    void render(RenderWindow& w) override {
        w.setView(w.getDefaultView());
        w.clear(Color::Black);
        w.draw(bg);
        w.draw(overlay);
        w.draw(title);

        // Highlight box around selected
        for (int i = 0; i < 3; i++) {
            if (i == selected) {
                RectangleShape highlight({280.f, 380.f});
                highlight.setFillColor(Color(0, 230, 255, 30));
                highlight.setOutlineColor(Color(0, 230, 255));
                highlight.setOutlineThickness(4.f);
                highlight.setPosition({G::SCREEN_W / 4.f * (i + 1) - 140.f,
                                       380.f});
                w.draw(highlight);

                labels[i].setFillColor(Color::Yellow);
            } else {
                labels[i].setFillColor(Color::White);
            }
        }

        for (int i = 0; i < 3; i++) {
            previews[i].draw(w);
            w.draw(labels[i]);
        }

        w.draw(controlHint);
        w.display();
    }
};
