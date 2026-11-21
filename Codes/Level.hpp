#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"
#include "AnimatedSprite.hpp"
#include "Tilemap.hpp"
#include "Parallax.hpp"
#include "Player.hpp"
#include "Entity.hpp"
#include "Enemy.hpp"
#include "HUD.hpp"
#include "SoundManager.hpp"

// ════════════════════════════════════════════════════════════════
//  Level — the core gameplay container.
//
//  Owns:
//    * Tilemap (collision grid)
//    * Parallax background
//    * Player
//    * Vector of Entity (pickups, hazards)
//    * Vector of Enemy (with virtual update/AI)
//    * View / camera
//    * HUD
//    * Goal location
//    * Time limit
//
//  Public API: setup(theme, layout, ...), update(dt), render(window).
//  Game-state flags: won, lost, complete.
// ════════════════════════════════════════════════════════════════
class Level {
public:
    enum class Theme { Industrial, Green };

    // Public flags read by the surrounding GameState
    bool won  = false;
    bool lost = false;
    int  score = 0;
    int  coinsCollected = 0;

private:
    // ── Theme paths ─────────────────────────────────────────────
    Theme  theme_ = Theme::Industrial;
    string themeRoot_;

    // ── World ───────────────────────────────────────────────────
    Tilemap          map_;
    Parallax         parallax_;
    Player*          player_ = nullptr;     // borrowed; CharSelect owns
    vector<unique_ptr<Entity>> entities_;
    vector<unique_ptr<Enemy>>  enemies_;

    // ── Visual: tile texture lookup ─────────────────────────────
    Texture* groundTex_   = nullptr;
    Texture* groundTexB_  = nullptr;   // alternate variant
    Texture* platformTex_ = nullptr;
    Texture* platformTopTex_ = nullptr;

    // ── Decorative sprites placed in the world ─────────────────
    vector<Sprite> decorations_;
    vector<AnimatedSprite> animatedDecor_;

    // ── Solid colliders for decorations (boxes, barrels, lockers…)
    // Tight bounding box of the non-transparent pixels of a deco
    // sprite, used to block the player in collideX / collideY.
    vector<FloatRect> decoColliders_;

    // ── Goal ────────────────────────────────────────────────────
    Vector2f goalPos_  {0.f, 0.f};
    optional<AnimatedSprite> goalAnim_;

    // ── Camera ──────────────────────────────────────────────────
    View view_;
    float camX_ = G::SCREEN_W / 2.f;
    float camY_ = G::SCREEN_H / 2.f;

    // ── Timer ───────────────────────────────────────────────────
    float timeLimit_   = 180.f;
    float timeRemaining_ = 180.f;

    // ── HUD ─────────────────────────────────────────────────────
    HUD hud_;
    string levelLabel_;

    // ── Background flat colour for clear ────────────────────────
    Color bgColor_ = Color(15, 15, 30);

    // ── Helpers ─────────────────────────────────────────────────
    static FloatRect playerBox(const Player& p) { return p.hitbox(); }

    // Resolve player vs tilemap horizontally
    void collideX(float dx) {
        player_->moveX(dx);
        FloatRect pb = player_->hitbox();
        // Test against tiles overlapping pb
        Vector2i tlMin = map_.worldToTile({pb.position.x, pb.position.y});
        Vector2i tlMax = map_.worldToTile({pb.position.x + pb.size.x,
                                           pb.position.y + pb.size.y});
        for (int r = tlMin.y; r <= tlMax.y; r++) {
            for (int c = tlMin.x; c <= tlMax.x; c++) {
                char t = map_.at(c, r);
                if (!map_.isSolid(t)) continue;
                FloatRect tr = map_.tileRect(c, r);
                auto inter = pb.findIntersection(tr);
                if (inter) {
                    if (dx > 0.f) player_->moveX(-inter->size.x);
                    else if (dx < 0.f) player_->moveX(inter->size.x);
                    pb = player_->hitbox();
                }
            }
        }
        // Decoration colliders (boxes, barrels, lockers…) are also
        // solid so the player can't walk straight through them.
        for (const auto& dr : decoColliders_) {
            auto inter = pb.findIntersection(dr);
            if (inter) {
                if (dx > 0.f) player_->moveX(-inter->size.x);
                else if (dx < 0.f) player_->moveX(inter->size.x);
                pb = player_->hitbox();
            }
        }
    }

    // Resolve player vs tilemap vertically
    void collideY(float dy) {
        player_->moveY(dy);
        FloatRect pb = player_->hitbox();
        Vector2i tlMin = map_.worldToTile({pb.position.x, pb.position.y});
        Vector2i tlMax = map_.worldToTile({pb.position.x + pb.size.x,
                                           pb.position.y + pb.size.y});
        bool landed = false;
        for (int r = tlMin.y; r <= tlMax.y; r++) {
            for (int c = tlMin.x; c <= tlMax.x; c++) {
                char t = map_.at(c, r);
                FloatRect tr = map_.tileRect(c, r);
                if (map_.isSolid(t)) {
                    auto inter = pb.findIntersection(tr);
                    if (inter) {
                        if (dy > 0.f) {
                            // Falling — land
                            player_->moveY(-inter->size.y);
                            landed = true;
                        } else if (dy < 0.f) {
                            // Rising — hit ceiling
                            player_->moveY(inter->size.y);
                            player_->mutableVel().y = 0.f;
                        }
                        pb = player_->hitbox();
                    }
                } else if (map_.isOneWay(t)) {
                    // One-way: only collide when descending and player's
                    // bottom was above the tile's top in the previous frame
                    if (dy > 0.f) {
                        float tileTop = tr.position.y;
                        float playerBot = pb.position.y + pb.size.y;
                        // Treat as solid only if we just crossed the top edge
                        if (playerBot - dy <= tileTop + 0.5f && playerBot >= tileTop) {
                            player_->moveY(tileTop - playerBot);
                            landed = true;
                            pb = player_->hitbox();
                        }
                    }
                }
            }
        }
        // Decoration colliders behave as full solids: block when
        // rising (head bump) AND when falling (player stands on top).
        for (const auto& dr : decoColliders_) {
            auto inter = pb.findIntersection(dr);
            if (inter) {
                if (dy > 0.f) {
                    player_->moveY(-inter->size.y);
                    landed = true;
                } else if (dy < 0.f) {
                    player_->moveY(inter->size.y);
                    player_->mutableVel().y = 0.f;
                }
                pb = player_->hitbox();
            }
        }
        player_->setOnGround(landed && dy >= 0.f);
        if (!landed && dy >= 0.f) player_->setOnGround(false);
    }

public:
    void setPlayer(Player* p) { player_ = p; }

    int  coins()         const { return coinsCollected; }
    int  totalScore()    const { return score; }
    float timeRemaining()const { return timeRemaining_; }

    // ════════════════════════════════════════════════════════════
    //  setup() — build the level for the given theme.
    //  Each theme has a fixed layout encoded as a tile-grid string.
    // ════════════════════════════════════════════════════════════
    void setupIndustrial() {
        theme_ = Theme::Industrial;
        themeRoot_ = "../assets/levels/industrial";
        levelLabel_ = "INDUSTRIAL  ZONE";
        timeLimit_ = 60.f;
        timeRemaining_ = timeLimit_;

        AssetCache& A = AssetCache::get();
        groundTex_      = &A.tex(themeRoot_ + "/tiles/IndustrialTile_01.png");
        groundTexB_     = &A.tex(themeRoot_ + "/tiles/IndustrialTile_02.png");
        platformTex_    = &A.tex(themeRoot_ + "/tiles/IndustrialTile_32.png");
        platformTopTex_ = &A.tex(themeRoot_ + "/tiles/IndustrialTile_31.png");

        parallax_.load(themeRoot_ + "/bg");

        // Layout: 11 rows × 110 cols.  Row 10 is ground.
        // Use #/ground tiles to build a varied side-scroller.
        vector<string> L = {
        //         1111111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000
        //01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
            "..........................................................................................................",
            "..........................................................................................................",
            "..........................................................................................................",
            "..........................................................................................................",
            "..............................-----..............................................-------..................",
            "...................-------......................--------..............-----...............................",
            "..........-------................................................-----...........................---------",
            "....-----.....................##.................................................##......................=",
            "...........................##======##.....................##======##....................................==",
            "..........================================================================================================",
            "==========================================================================================================",
        };
        map_.load(L);

        // Spawn the player just above tile (3, 9)
        if (player_) player_->setPosition({3 * G::PIXEL + G::PIXEL / 2.f,
                                             9 * G::PIXEL});

        // ── Place enemies (Walker on ground in 3 places) ────────
        EnemyFrameSpec e1 = {"../assets/enemies/e1", 4, 6, 6, 2, 6};
        EnemyFrameSpec e2 = {"../assets/enemies/e2", 4, 6, 9, 2, 6};
        enemies_.push_back(make_unique<Walker>(e1,
            Vector2f{20.f * G::PIXEL, 9.f * G::PIXEL},
            18.f * G::PIXEL, 28.f * G::PIXEL, 100.f));
        enemies_.push_back(make_unique<Walker>(e1,
            Vector2f{45.f * G::PIXEL, 9.f * G::PIXEL},
            42.f * G::PIXEL, 55.f * G::PIXEL, 110.f));

        // Charger that hunts the player
        auto ch = make_unique<Charger>(e2,
            Vector2f{75.f * G::PIXEL, 9.f * G::PIXEL},
            70.f * G::PIXEL, 85.f * G::PIXEL);
        ch->setTarget(player_);
        enemies_.push_back(std::move(ch));

        enemies_.push_back(make_unique<Walker>(e1,
            Vector2f{95.f * G::PIXEL, 9.f * G::PIXEL},
            92.f * G::PIXEL, 100.f * G::PIXEL, 100.f));

        // ── Place coins ─────────────────────────────────────────
        auto addCoin = [&](float col, float row) {
            entities_.push_back(make_unique<Coin>(
                Vector2f{col * G::PIXEL + G::PIXEL/2.f, row * G::PIXEL}));
        };
        // Floating coins above gaps and on/over platforms
        for (int i = 8; i < 14; i++)  addCoin(i, 6);
        for (int i = 19; i < 26; i++) addCoin(i, 5);
        for (int i = 34; i < 39; i++) addCoin(i, 5);
        for (int i = 50; i < 58; i++) addCoin(i, 4);
        for (int i = 64; i < 70; i++) addCoin(i, 6);
        for (int i = 78; i < 83; i++) addCoin(i, 5);
        for (int i = 88; i < 93; i++) addCoin(i, 4);

        // Cards for big score
        entities_.push_back(make_unique<Card>(Vector2f{38.f * G::PIXEL, 4.f * G::PIXEL}));
        entities_.push_back(make_unique<Card>(Vector2f{82.f * G::PIXEL, 4.f * G::PIXEL}));

        // Health pickup at midpoint
        entities_.push_back(make_unique<HealthPickup>(Vector2f{42.f * G::PIXEL, 8.f * G::PIXEL}));

        // ── Goal: animated "Entry" portal at the right edge ─────
        Texture& gt = A.tex(themeRoot_ + "/animated/Entry.png");
        goalAnim_.emplace(gt, 4, 0.18f);
        goalAnim_->setScale(3.f);
        goalPos_ = {(map_.width - 3.f) * G::PIXEL,  9.f * G::PIXEL};
        goalAnim_->setPosition(goalPos_);

        // ── Decorations ─────────────────────────────────────────
        // Industrial objects — boxes, barrels, lockers, fire ext, bench.
        // `solid=true` ⇒ a tight non-transparent bbox is added to
        // decoColliders_ so the player is physically blocked, instead
        // of jumping/walking straight through tall obstacles.
        decorations_.clear();
        decoColliders_.clear();
        auto putDeco = [&](const string& name, float col, float row,
                           float scale = 3.f, bool solid = false) {
            try {
                Texture& t = A.tex(themeRoot_ + "/objects/" + name + ".png");
                Sprite s(t);
                s.setScale({scale, scale});
                float h = (float)t.getSize().y * scale;
                Vector2f spritePos{col * G::PIXEL, row * G::PIXEL - h};
                s.setPosition(spritePos);
                decorations_.push_back(s);

                if (solid) {
                    // Compute the tight bbox of non-transparent pixels
                    // once at load time, in world coordinates.
                    Image img = t.copyToImage();
                    Vector2u sz = img.getSize();
                    int minX = (int)sz.x, minY = (int)sz.y;
                    int maxX = -1, maxY = -1;
                    for (unsigned y = 0; y < sz.y; ++y) {
                        for (unsigned x = 0; x < sz.x; ++x) {
                            if (img.getPixel({x, y}).a > 0) {
                                if ((int)x < minX) minX = (int)x;
                                if ((int)x > maxX) maxX = (int)x;
                                if ((int)y < minY) minY = (int)y;
                                if ((int)y > maxY) maxY = (int)y;
                            }
                        }
                    }
                    if (maxX >= 0) {
                        FloatRect rc(
                            {spritePos.x + minX * scale,
                             spritePos.y + minY * scale},
                            {(maxX - minX + 1) * scale,
                             (maxY - minY + 1) * scale});
                        decoColliders_.push_back(rc);
                    }
                }
            } catch (...) {}
        };
        // The bench + bucket sit by the spawn — left non-solid so the
        // player isn't fenced in at the start.
        putDeco("Bench",    1.5f, 9);
        putDeco("Bucket",   2.5f, 9);
        // The standing-broom decoration that used to be at col 2.8
        // rendered floating above the bucket — removed.
        putDeco("Box1",     6.5f, 9, 3.f, true);
        putDeco("Barrel1",  16.f, 9, 3.f, true);
        putDeco("Box2",     31.f, 9, 3.f, true);
        putDeco("Barrel2",  41.f, 9, 3.f, true);
        putDeco("Locker1",  60.f, 9, 3.f, true);
        putDeco("Locker2",  60.7f, 9, 3.f, true);
        putDeco("Locker3",  61.4f, 9, 3.f, true);
        putDeco("Fire-extinguisher1", 73.f, 9, 3.f, true);
        putDeco("Box3",     85.f, 9, 3.f, true);
        putDeco("Pointer1", 102.f, 9, 3.f, true);

        bgColor_ = Color(15, 15, 30);
        view_.setSize({G::SCREEN_W, G::SCREEN_H});
    }

    void setupGreen() {
        theme_ = Theme::Green;
        themeRoot_ = "../assets/levels/green";
        levelLabel_ = "GREEN  ZONE";
        timeLimit_ = 40.f;
        timeRemaining_ = timeLimit_;

        AssetCache& A = AssetCache::get();
        groundTex_      = &A.tex(themeRoot_ + "/tiles/Tile_01.png");
        groundTexB_     = &A.tex(themeRoot_ + "/tiles/Tile_02.png");
        platformTex_    = &A.tex(themeRoot_ + "/tiles/Tile_32.png");
        platformTopTex_ = &A.tex(themeRoot_ + "/tiles/Tile_31.png");

        parallax_.load(themeRoot_ + "/bg");

        // A more vertical / branching layout for level 2
        vector<string> L = {
            "..........................................................................................................",
            "..........................................................................................................",
            "...........................................######...............................................---------",
            ".....................-----..........................................######.......######....##............",
            "............#####...........................................................................................",
            "....---..............................########..........-----...........................................##.",
            "................................................................##====##......-----.....................==",
            "............................##========##............##====##.................................==..........==",
            "....##====##.......##====##............................................................##====##..........==",
            "==========================================================================================================.",
            "==========================================================================================================.",
        };
        map_.load(L);

        if (player_) player_->setPosition({3 * G::PIXEL + G::PIXEL / 2.f,
                                             9 * G::PIXEL});

        // Tougher enemy mix for level 2:
        //  • Faster walkers (140-160 px/s vs level 1's 100-110)
        //  • Three chargers instead of two
        //  • One extra walker near the goal as a final gauntlet
        EnemyFrameSpec e3 = {"../assets/enemies/e3", 4, 6, 4, 2, 6};
        EnemyFrameSpec e4 = {"../assets/enemies/e4", 4, 4, 4, 2, 4};
        EnemyFrameSpec e5 = {"../assets/enemies/e5", 4, 4, 4, 2, 4};

        enemies_.push_back(make_unique<Walker>(e3,
            Vector2f{18.f * G::PIXEL, 9.f * G::PIXEL},
            14.f * G::PIXEL, 24.f * G::PIXEL, 150.f));

        auto ch1 = make_unique<Charger>(e4,
            Vector2f{34.f * G::PIXEL, 9.f * G::PIXEL},
            30.f * G::PIXEL, 42.f * G::PIXEL,
            100.f, 340.f, 500.f);   // walk, charge, vision
        ch1->setTarget(player_);
        enemies_.push_back(std::move(ch1));

        enemies_.push_back(make_unique<Walker>(e5,
            Vector2f{52.f * G::PIXEL, 9.f * G::PIXEL},
            48.f * G::PIXEL, 62.f * G::PIXEL, 160.f));

        auto ch2 = make_unique<Charger>(e4,
            Vector2f{70.f * G::PIXEL, 9.f * G::PIXEL},
            64.f * G::PIXEL, 78.f * G::PIXEL,
            100.f, 340.f, 500.f);
        ch2->setTarget(player_);
        enemies_.push_back(std::move(ch2));

        enemies_.push_back(make_unique<Walker>(e3,
            Vector2f{85.f * G::PIXEL, 9.f * G::PIXEL},
            82.f * G::PIXEL, 92.f * G::PIXEL, 150.f));

        auto ch3 = make_unique<Charger>(e5,
            Vector2f{100.f * G::PIXEL, 9.f * G::PIXEL},
            95.f * G::PIXEL, 104.f * G::PIXEL,
            120.f, 380.f, 550.f);   // final-stretch charger is the meanest
        ch3->setTarget(player_);
        enemies_.push_back(std::move(ch3));

        // Coins: more challenging placements (require double jump)
        auto addCoin = [&](float c, float r) {
            entities_.push_back(make_unique<Coin>(
                Vector2f{c * G::PIXEL + G::PIXEL/2.f, r * G::PIXEL}));
        };
        for (int i = 6; i < 12; i++) addCoin(i, 7);
        for (int i = 23; i < 30; i++) addCoin(i, 4);
        for (int i = 40; i < 48; i++) addCoin(i, 6);
        for (int i = 56; i < 62; i++) addCoin(i, 5);
        for (int i = 68; i < 75; i++) addCoin(i, 7);
        for (int i = 80; i < 90; i++) addCoin(i, 4);

        entities_.push_back(make_unique<Card>(Vector2f{45.f * G::PIXEL, 5.f * G::PIXEL}));
        entities_.push_back(make_unique<Card>(Vector2f{86.f * G::PIXEL, 3.f * G::PIXEL}));
        entities_.push_back(make_unique<HealthPickup>(Vector2f{55.f * G::PIXEL, 6.f * G::PIXEL}));
        entities_.push_back(make_unique<HealthPickup>(Vector2f{96.f * G::PIXEL, 4.f * G::PIXEL}));

        // Goal: green theme has Card animation; reuse it as the goal
        try {
            Texture& gt = A.tex(themeRoot_ + "/animated/Fountain.png");
            goalAnim_.emplace(gt, 4, 0.20f);
            goalAnim_->setScale(2.5f);
        } catch (...) {
            // Fallback: use Card
            Texture& gt = A.tex(themeRoot_ + "/animated/Card.png");
            goalAnim_.emplace(gt, 8, 0.10f);
            goalAnim_->setScale(2.5f);
        }
        goalPos_ = {(map_.width - 3.f) * G::PIXEL, 9.f * G::PIXEL};
        goalAnim_->setPosition(goalPos_);

        // Green decorations
        decorations_.clear();
        decoColliders_.clear();
        auto putDeco = [&](const string& name, float col, float row, float scale = 3.f) {
            try {
                Texture& t = A.tex(themeRoot_ + "/objects/" + name + ".png");
                Sprite s(t);
                s.setScale({scale, scale});
                float h = (float)t.getSize().y * scale;
                s.setPosition({col * G::PIXEL, row * G::PIXEL - h});
                decorations_.push_back(s);
            } catch (...) {}
        };
        // Green objects use numbered files (bushes/grass/stones merged)
        // Place a variety of decorations along the ground row
        const char* greenDecoNames[] = {"1", "2", "3", "4", "5", "6"};
        for (int i = 0; i < 12; i++) {
            const char* n = greenDecoNames[i % 6];
            putDeco(n, 4.f + i * 8.f, 9, 3.f);
        }

        bgColor_ = Color(20, 30, 40);
        view_.setSize({G::SCREEN_W, G::SCREEN_H});
    }

    void initHUD() {
        hud_.init("../assets/CyberpunkCraftpixPixel.otf");
    }

    // ════════════════════════════════════════════════════════════
    //  EVENTS — called from outer GameState
    // ════════════════════════════════════════════════════════════
    void onJumpPressed()   { if (player_) player_->onJumpPressed(); }
    void onAttackPressed() { if (player_) player_->onAttackPressed(); }

    // ════════════════════════════════════════════════════════════
    //  UPDATE — physics, collision, AI, timer, win/lose
    // ════════════════════════════════════════════════════════════
    void update(float dt) {
        if (!player_) return;

        if (won || lost) {
            // Still animate things visually
            if (goalAnim_) goalAnim_->update(dt);
            for (auto& e : entities_) e->update(dt);
            for (auto& en: enemies_)  en->update(dt);
            return;
        }

        // ── Timer ──────────────────────────────────────────────
        timeRemaining_ -= dt;
        if (timeRemaining_ <= 0.f) {
            timeRemaining_ = 0.f;
            lost = true;
            SoundManager::get().playGameOver();
            return;
        }

        // ── Player input & physics ─────────────────────────────
        player_->handleInput();
        player_->update(dt);

        // Apply velocity in two phases for proper AABB resolution
        Vector2f& v = player_->mutableVel();
        collideX(v.x * dt);
        collideY(v.y * dt);

        // Death from falling out of map — instant game over
        if (player_->position().y > map_.worldH() + 200.f) {
            lost = true;
            SoundManager::get().playGameOver();
            return;
        }

        if (player_->isDead()) {
            lost = true;
            SoundManager::get().playGameOver();
            return;
        }

        // ── Enemies update + interactions ───────────────────────
        for (auto& en : enemies_) en->update(dt);

        FloatRect pb = player_->hitbox();
        FloatRect ab = player_->attackBox();

        // Enemy-vs-player overlap → damage to player
        for (auto& en : enemies_) {
            if (!en->alive()) continue;
            if (!en->isHazardous()) continue;
            if (pb.findIntersection(en->bounds())) {
                en->onPlayerHit(*player_);
            }
        }

        // Player-attack-vs-enemy overlap → damage to enemy
        if (player_->attackUnconsumed()) {
            for (auto& en : enemies_) {
                if (!en->alive()) continue;
                if (en->isHazardous() && ab.findIntersection(en->bounds())) {
                    en->takeAttack(1, player_->position().x);
                    player_->markAttackConsumed();
                    score += 25;
                    break;
                }
            }
        }

        // ── Pickups ─────────────────────────────────────────────
        for (auto& e : entities_) {
            e->update(dt);
            if (!e->alive() || !e->isPickup()) continue;
            if (pb.findIntersection(e->bounds())) {
                if (auto* coin = dynamic_cast<Coin*>(e.get())) {
                    coin->onPlayerHit(*player_);
                    score += coin->scoreValue();
                    coinsCollected += 1;
                }
                else if (auto* card = dynamic_cast<Card*>(e.get())) {
                    card->onPlayerHit(*player_);
                    score += card->scoreValue();
                    timeRemaining_ += card->timeBonus();
                    hud_.showMessage("KEYCARD  +" + to_string(card->timeBonus()) + "s", 1.5f);
                }
                else if (auto* hp = dynamic_cast<HealthPickup*>(e.get())) {
                    int d = hp->hpDelta();
                    hp->onPlayerHit(*player_);
                    // Restore HP without going past max — easiest: a one-off heal
                    // We expose health() but no addHealth; cheat by calling
                    // takeDamage(-d) — but player guards against death.
                    // Cleaner: add dedicated heal method to Player. We'll do so
                    // inline here using a friend-ish hack:
                    // (Player class has reset() but not heal() — added below.)
                    player_->heal(d);
                    hud_.showMessage("HEALTH  RESTORED", 1.2f);
                }
                else if (auto* ch = dynamic_cast<Chest*>(e.get())) {
                    ch->onPlayerHit(*player_);
                    score += 100;
                }
            }
        }

        // ── Goal reached? ───────────────────────────────────────
        if (goalAnim_) {
            goalAnim_->update(dt);
            FloatRect gb({goalPos_.x - G::PIXEL/2.f,
                          goalPos_.y - G::PIXEL * 1.5f},
                         {G::PIXEL, G::PIXEL * 1.5f});
            if (pb.findIntersection(gb)) {
                won = true;
                // Time-bonus: every remaining second worth +5
                score += (int)timeRemaining_ * 5;
                SoundManager::get().playWin();
                SoundManager::get().stopMusic();
            }
        }

        // ── Camera follow ───────────────────────────────────────
        // Guard against levels whose world is smaller than the screen.
        // std::clamp asserts when lo > hi; we pin the camera to the
        // world's centre in that axis instead.
        float halfW = G::SCREEN_W / 2.f;
        float halfH = G::SCREEN_H / 2.f;
        float worldW = map_.worldW();
        float worldH = map_.worldH();

        float targetX, targetY;
        if (worldW <= G::SCREEN_W) {
            targetX = worldW / 2.f;
        } else {
            targetX = clamp(player_->position().x, halfW, worldW - halfW);
        }
        if (worldH <= G::SCREEN_H) {
            targetY = worldH / 2.f;
        } else {
            targetY = clamp(player_->position().y - 200.f,
                            halfH, worldH - halfH);
        }
        camX_ += (targetX - camX_) * min(1.f, dt * 6.f);
        camY_ += (targetY - camY_) * min(1.f, dt * 4.f);
        view_.setCenter({camX_, camY_});

        parallax_.setCamera(camX_);

        // HUD message timer
        hud_.update(dt);
    }

    // ════════════════════════════════════════════════════════════
    //  RENDER
    void render(RenderWindow& w) {
        w.clear(bgColor_);

        // Parallax in screen-space
        w.setView(w.getDefaultView());
        parallax_.render(w);

        // World view
        w.setView(view_);

        // Tiles
        renderTiles(w);

        // Decorative sprites (drawn over tiles)
        for (auto& s : decorations_) w.draw(s);

        // Goal
        if (goalAnim_) goalAnim_->draw(w);

        // Entities (pickups)
        for (auto& e : entities_) e->render(w);

        // Enemies (death animations still draw)
        for (auto& en : enemies_) en->render(w);

        // Player on top
        if (player_) player_->render(w);

        // ── HUD in screen-space ─────────────────────────────────
        w.setView(w.getDefaultView());
        if (player_) hud_.drawHearts(w, player_->health(), G::PLAYER_MAX_HP);
        hud_.drawCoins(w, coinsCollected);
        hud_.drawScore(w, score);
        hud_.drawTimer(w, timeRemaining_);
        hud_.drawLevelLabel(w, levelLabel_);
        hud_.drawMessage(w);
    }

    // Render visible tiles only (for performance)
    void renderTiles(RenderWindow& w) {
        FloatRect viewRect({camX_ - G::SCREEN_W / 2.f,
                            camY_ - G::SCREEN_H / 2.f},
                           {G::SCREEN_W, G::SCREEN_H});
        int colMin = max(0, (int)floor(viewRect.position.x / G::PIXEL) - 1);
        int colMax = min(map_.width - 1,
                         (int)ceil((viewRect.position.x + viewRect.size.x) / G::PIXEL) + 1);
        int rowMin = max(0, (int)floor(viewRect.position.y / G::PIXEL) - 1);
        int rowMax = min(map_.height - 1,
                         (int)ceil((viewRect.position.y + viewRect.size.y) / G::PIXEL) + 1);

        for (int r = rowMin; r <= rowMax; r++) {
            for (int c = colMin; c <= colMax; c++) {
                char t = map_.at(c, r);
                if (t == '.') continue;
                Texture* tex = nullptr;
                if (t == '=') {
                    tex = ((c + r) % 5 == 0) ? groundTexB_ : groundTex_;
                } else if (t == '#') {
                    // Top row of platform uses platform-top, others mid
                    char above = map_.at(c, r - 1);
                    tex = (above == '#' || above == '=') ? platformTex_ : platformTopTex_;
                } else if (t == '-') {
                    tex = platformTopTex_;
                }
                if (!tex) continue;
                Sprite s(*tex);
                s.setScale({G::TILE_SCALE, G::TILE_SCALE});
                s.setPosition({c * G::PIXEL, r * G::PIXEL});
                w.draw(s);
            }
        }
    }
};