#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AnimatedSprite.hpp"
#include "AssetCache.hpp"
#include "SoundManager.hpp"

// ════════════════════════════════════════════════════════════════
//  Entity — abstract base for everything in the game world that
//  isn't the player or a static tile.
//
//  Polymorphism root #2: Level holds a vector<unique_ptr<Entity>>
//  and dispatches update/render/onPlayerHit through virtuals.
// ════════════════════════════════════════════════════════════════
class Entity {
protected:
    bool      alive_ = true;
    FloatRect bounds_;

public:
    virtual ~Entity() = default;

    virtual void update(float dt)            = 0;
    virtual void render(RenderWindow& w)     = 0;

    // Default: not a pickup, not hostile, doesn't damage
    virtual bool isPickup()           const { return false; }
    virtual bool isHazardous()        const { return false; }
    virtual int  damageAmount()       const { return 0; }

    // Called when player's hitbox overlaps this entity's bounds
    virtual void onPlayerHit(class Player& /*p*/) {}

    // Called by level after player attack-box overlaps this enemy
    virtual void takeAttack(int /*dmg*/, float /*fromX*/) {}

    bool       alive()  const { return alive_; }
    FloatRect  bounds() const { return bounds_; }
    void       setAlive(bool b) { alive_ = b; }
};

// ════════════════════════════════════════════════════════════════
//  Pickup — base for collectibles. Subclasses override scoring and
//  the on-collect side effect.
// ════════════════════════════════════════════════════════════════
class Pickup : public Entity {
protected:
    AnimatedSprite anim;
    float          baseY;
    float          bobTime = 0.f;

public:
    Pickup(const Texture& tex, int frames, Vector2f pos, float scale = 2.5f)
        : anim(tex, frames, 0.10f)
    {
        anim.setScale(scale);
        anim.setPosition(pos);
        baseY = pos.y;
        // Bottom-centre origin so y is the bottom of the sprite — for
        // a pickup we re-anchor the bounds box on the centre of the visible
        // sprite, so build bounds from sprite size.
        Vector2u s = tex.getSize();
        float fw = (float)s.x / max(1, frames) * scale;
        float fh = (float)s.y * scale;
        bounds_ = FloatRect({pos.x - fw / 2.f, pos.y - fh}, {fw, fh});
    }

    bool isPickup() const override { return true; }

    void update(float dt) override {
        if (!alive_) return;
        anim.update(dt);
        bobTime += dt;
        float y = baseY + sin(bobTime * 3.f) * 6.f;
        anim.setPosition({bounds_.position.x + bounds_.size.x / 2.f, y});
        bounds_.position.y = y - bounds_.size.y;
    }

    void render(RenderWindow& w) override { if (alive_) anim.draw(w); }
};

// Coin: gives +1 score, plays coin SFX
class Coin : public Pickup {
public:
    Coin(Vector2f pos)
        : Pickup(AssetCache::get().tex("../assets/levels/industrial/animated/Money.png"),
                 6, pos, 2.5f) {}

    void onPlayerHit(Player&) override {
        if (!alive_) return;
        alive_ = false;
        SoundManager::get().playCoin();
    }

    int scoreValue() const { return 10; }
};

// Card: rare key-card, +50 score, gives extra time
class Card : public Pickup {
public:
    Card(Vector2f pos)
        : Pickup(AssetCache::get().tex("../assets/levels/industrial/animated/Card.png"),
                 8, pos, 2.5f) {}

    void onPlayerHit(Player&) override {
        if (!alive_) return;
        alive_ = false;
        SoundManager::get().playCoin();
    }

    int  scoreValue()  const { return 50; }
    int  timeBonus()   const { return 10; }   // +10s
};

// HealthPickup: +1 hp, uses an icon from the GUI pack
class HealthPickup : public Pickup {
public:
    HealthPickup(Vector2f pos)
        : Pickup(AssetCache::get().tex("../assets/gui/icons/Icon_28.png"),
                 1, pos, 2.f) {}

    void onPlayerHit(Player& p) override {
        if (!alive_) return;
        alive_ = false;
        SoundManager::get().playCoin();
        // Add HP via reset trick — actually we don't want full reset.
        // We expose a hpDelta() instead and let Level apply it.
    }
    int hpDelta() const { return 1; }
};

// Chest: opens animation when player touches, drops coins
class Chest : public Entity {
    AnimatedSprite anim;
    bool opened = false;
public:
    int coinsInside = 5;
    Chest(Vector2f pos)
        : anim(AssetCache::get().tex("../assets/levels/industrial/animated/Chest.png"),
               8, 0.10f, false)
    {
        anim.setScale(3.f);
        anim.setPosition(pos);
        Vector2u s = AssetCache::get().tex("../assets/levels/industrial/animated/Chest.png").getSize();
        float fw = (float)s.x / 8.f * 3.f;
        float fh = (float)s.y * 3.f;
        bounds_ = FloatRect({pos.x - fw / 2.f, pos.y - fh}, {fw, fh});
    }

    bool wasOpened() const { return opened; }
    bool isPickup() const override { return true; }

    void update(float dt) override {
        if (opened) anim.update(dt);
    }
    void render(RenderWindow& w) override { anim.draw(w); }

    void onPlayerHit(Player&) override {
        if (!opened) {
            opened = true;
            SoundManager::get().playCoin();
        }
    }
};
