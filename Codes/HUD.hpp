#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"

// ════════════════════════════════════════════════════════════════
//  HUD — draws health hearts, coin counter, level timer, score.
//  Drawn in screen-space (caller must reset to default view first).
// ════════════════════════════════════════════════════════════════
class HUD {
    Font* font = nullptr;
    Texture* heartFull  = nullptr;
    Texture* heartEmpty = nullptr;
    Texture* coinIcon   = nullptr;
    Texture* frameTex   = nullptr;

    // Cached text objects (constructed on init)
    optional<Text> coinText;
    optional<Text> scoreText;
    optional<Text> timerText;
    optional<Text> levelText;
    optional<Text> messageText;     // optional flash message

    float messageTimer = 0.f;

public:
    void init(const string& fontPath) {
        AssetCache& A = AssetCache::get();
        font       = &A.font(fontPath);
        heartFull  = &A.tex("../assets/gui/icons/Icon_38.png");  // heart-ish icon
        heartEmpty = &A.tex("../assets/gui/icons/Icon_39.png");
        coinIcon   = &A.tex("../assets/gui/icons/Icon_30.png");  // coin/star icon
        frameTex   = &A.tex("../assets/gui/frames/Frame_01.png");

        coinText.emplace(*font);    coinText->setCharacterSize(36);
        coinText->setFillColor(Color::Yellow);
        coinText->setOutlineColor(Color::Black);
        coinText->setOutlineThickness(2.f);

        scoreText.emplace(*font);   scoreText->setCharacterSize(34);
        scoreText->setFillColor(Color(0, 230, 255));
        scoreText->setOutlineColor(Color::Black);
        scoreText->setOutlineThickness(2.f);

        timerText.emplace(*font);   timerText->setCharacterSize(46);
        timerText->setFillColor(Color::Cyan);
        timerText->setOutlineColor(Color::Black);
        timerText->setOutlineThickness(3.f);

        levelText.emplace(*font);   levelText->setCharacterSize(40);
        levelText->setFillColor(Color(0, 230, 255));
        levelText->setOutlineColor(Color::Black);
        levelText->setOutlineThickness(2.f);

        messageText.emplace(*font); messageText->setCharacterSize(64);
        messageText->setFillColor(Color::Yellow);
        messageText->setOutlineColor(Color::Black);
        messageText->setOutlineThickness(4.f);
    }

    void showMessage(const string& s, float seconds = 2.0f) {
        if (!messageText) return;
        messageText->setString(s);
        FloatRect b = messageText->getLocalBounds();
        messageText->setOrigin({b.size.x / 2.f, b.size.y / 2.f});
        messageText->setPosition({G::SCREEN_W / 2.f, 320.f});
        messageTimer = seconds;
    }

    void update(float dt) {
        if (messageTimer > 0.f) messageTimer -= dt;
    }

    // ── Draw hearts in top-left ─────────────────────────────────
    void drawHearts(RenderWindow& w, int hp, int maxHp) {
        if (!heartFull) return;
        const float scale = 3.f;
        const float gap = 6.f;
        float fw = (float)heartFull->getSize().x * scale;
        float x = 30.f;
        float y = 30.f;
        for (int i = 0; i < maxHp; i++) {
            Sprite h(i < hp ? *heartFull : *heartEmpty);
            h.setScale({scale, scale});
            h.setPosition({x, y});
            w.draw(h);
            x += fw + gap;
        }
    }

    // ── Draw "x123" coin counter in top-right ─────────────────
    void drawCoins(RenderWindow& w, int coins) {
        if (!coinIcon) return;
        const float scale = 3.f;
        Sprite ic(*coinIcon);
        ic.setScale({scale, scale});
        float iw = (float)coinIcon->getSize().x * scale;
        ic.setPosition({G::SCREEN_W - 220.f, 32.f});
        w.draw(ic);

        if (coinText) {
            coinText->setString("x " + to_string(coins));
            coinText->setPosition({G::SCREEN_W - 220.f + iw + 12.f, 38.f});
            w.draw(*coinText);
        }
    }

    // ── Score panel (under coin counter) ───────────────────────
    void drawScore(RenderWindow& w, int score) {
        if (!scoreText) return;
        scoreText->setString("SCORE  " + to_string(score));
        FloatRect b = scoreText->getLocalBounds();
        scoreText->setPosition({G::SCREEN_W - b.size.x - 30.f, 90.f});
        w.draw(*scoreText);
    }

    // ── Timer top-centre ───────────────────────────────────────
    void drawTimer(RenderWindow& w, float seconds) {
        if (!timerText) return;
        int m = (int)seconds / 60;
        int s = (int)seconds % 60;
        ostringstream ss;
        ss << setfill('0') << setw(2) << m << ":" << setw(2) << s;
        timerText->setString(ss.str());
        if      (seconds < 10.f) timerText->setFillColor(Color::Red);
        else if (seconds < 30.f) timerText->setFillColor(Color::Yellow);
        else                     timerText->setFillColor(Color::Cyan);
        FloatRect b = timerText->getLocalBounds();
        timerText->setPosition({G::SCREEN_W / 2.f - b.size.x / 2.f, 25.f});
        w.draw(*timerText);
    }

    // ── Level badge (top centre, below timer) ──────────────────
    void drawLevelLabel(RenderWindow& w, const string& s) {
        if (!levelText) return;
        levelText->setString(s);
        FloatRect b = levelText->getLocalBounds();
        levelText->setPosition({G::SCREEN_W / 2.f - b.size.x / 2.f, 90.f});
        w.draw(*levelText);
    }

    // ── Centre flash message (boss spawn etc.) ─────────────────
    void drawMessage(RenderWindow& w) {
        if (messageTimer > 0.f && messageText) {
            uint8_t a = (uint8_t)min(255.f, messageTimer * 200.f);
            Color c = messageText->getFillColor();
            messageText->setFillColor(Color(c.r, c.g, c.b, a));
            w.draw(*messageText);
        }
    }
};
