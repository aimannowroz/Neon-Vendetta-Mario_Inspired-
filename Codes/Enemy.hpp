#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "Entity.hpp"
#include "Player.hpp"

// ════════════════════════════════════════════════════════════════
//  Enemy — concrete-derived from Entity, but itself abstract
//  because subclasses define the AI behaviour in update().
//
//  Five animation states: IDLE, WALK, ATTACK, HURT, DEATH.
//  Same enum positions for every enemy.
// ════════════════════════════════════════════════════════════════
enum class EnemyState { Idle, Walk, Attack, Hurt, Death };

struct EnemyFrameSpec {
    string folder;       // e.g. "../assets/enemies/e1"
    int idle, walk, attack, hurt, death;
};

class Enemy : public Entity {
protected:
    vector<AnimatedSprite> anims;
    EnemyState state = EnemyState::Walk;
    Vector2f   pos;
    Vector2f   vel{0.f, 0.f};
    bool       facingRight = false;
    int        hp = 2;
    float      hurtTimer  = 0.f;
    float      attackCD   = 0.f;
    float      hitFlash   = 0.f;        // visual red flash

    float minX, maxX;                   // patrol bounds
    float footY;                        // baseline y the enemy walks on

    // Tight hitbox (matches the previous Player calibration ratio)
    static constexpr float HBOX_W_RAW = 18.f;
    static constexpr float HBOX_H_RAW = 33.f;
    float scale_;

public:
    Enemy(const EnemyFrameSpec& spec, Vector2f startPos,
          float patrolMin, float patrolMax, float scale = 3.f)
        : pos(startPos), minX(patrolMin), maxX(patrolMax),
          footY(startPos.y), scale_(scale)
    {
        AssetCache& A = AssetCache::get();
        anims.emplace_back(A.tex(spec.folder + "/Idle.png"),   spec.idle,  0.15f);
        anims.emplace_back(A.tex(spec.folder + "/Walk.png"),   spec.walk,  0.10f);
        anims.emplace_back(A.tex(spec.folder + "/Attack.png"), spec.attack,0.08f, false);
        anims.emplace_back(A.tex(spec.folder + "/Hurt.png"),   spec.hurt,  0.10f, false);
        anims.emplace_back(A.tex(spec.folder + "/Death.png"),  spec.death, 0.12f, false);
        for (auto& a : anims) a.setScale(scale);

        bounds_ = FloatRect({pos.x - HBOX_W_RAW * scale / 2.f,
                              pos.y - HBOX_H_RAW * scale},
                             {HBOX_W_RAW * scale, HBOX_H_RAW * scale});
    }

    bool isHazardous() const override { return state != EnemyState::Death; }
    int  damageAmount() const override { return 1; }

    void setState(EnemyState s) {
        if (state == s) return;
        state = s;
        anims[(int)state].reset();
    }

    void takeAttack(int dmg, float fromX) override {
        if (state == EnemyState::Death) return;
        hp -= dmg;
        hurtTimer = 0.4f;
        hitFlash = 0.4f;
        // Knockback
        vel.x = (pos.x < fromX ? -1.f : 1.f) * 200.f;
        vel.y = -200.f;
        SoundManager::get().playHit();
        if (hp <= 0) {
            setState(EnemyState::Death);
            // Make non-hazardous but still drawn
            // alive_ stays true so render keeps drawing the death animation;
            // collision queries use isHazardous() which checks state.
        } else {
            setState(EnemyState::Hurt);
        }
    }

    void onPlayerHit(Player& p) override {
        if (state == EnemyState::Death) return;
        p.takeDamage(damageAmount(), pos.x);
    }

    Vector2f position() const { return pos; }

    // ── Update is purely physics + animation here. AI in subclasses. ──
    void physicsAndAnim(float dt) {
        if (hurtTimer > 0.f) hurtTimer -= dt;
        if (attackCD  > 0.f) attackCD  -= dt;
        if (hitFlash  > 0.f) hitFlash  -= dt;

        // Apply gravity for all states except death (which freezes)
        if (state != EnemyState::Death) {
            vel.y += G::GRAVITY * dt;
            if (vel.y > 1500.f) vel.y = 1500.f;
            pos += vel * dt;

            // Hard floor at footY (enemies don't navigate platforms)
            if (pos.y > footY) {
                pos.y = footY;
                vel.y = 0.f;
            }

            // Patrol bounds — flip direction when hitting an edge
            if (pos.x < minX) { pos.x = minX; facingRight = true;  vel.x = abs(vel.x); }
            if (pos.x > maxX) { pos.x = maxX; facingRight = false; vel.x = -abs(vel.x); }

            // Friction-ish: knockback decays
            if (state == EnemyState::Hurt) {
                vel.x *= (1.f - dt * 4.f);
            }
        }

        // Hurt → Idle when animation finishes
        if (state == EnemyState::Hurt && anims[3].finished)
            setState(EnemyState::Idle);

        // Bounds follow position
        bounds_.position.x = pos.x - bounds_.size.x / 2.f;
        bounds_.position.y = pos.y - bounds_.size.y;

        // Drive animation
        anims[(int)state].update(dt);
        anims[(int)state].setPosition(pos);
        anims[(int)state].setScale(scale_, !facingRight);
    }

    void render(RenderWindow& w) override {
        // Red flash overlay during hit
        if (hitFlash > 0.f) {
            // We don't have a shader system; just draw normally and let
            // the brief frame visually convey the hit.
        }
        anims[(int)state].draw(w);
    }
};

// ════════════════════════════════════════════════════════════════
//  Walker — patrols left/right, attacks player when in range.
// ════════════════════════════════════════════════════════════════
class Walker : public Enemy {
    float walkSpeed;
public:
    Walker(const EnemyFrameSpec& spec, Vector2f start,
           float minX, float maxX, float speed = 90.f)
        : Enemy(spec, start, minX, maxX), walkSpeed(speed)
    {
        vel.x = -walkSpeed;
        facingRight = false;
        setState(EnemyState::Walk);
    }

    void update(float dt) override {
        if (state != EnemyState::Death &&
            state != EnemyState::Hurt  &&
            state != EnemyState::Attack)
        {
            // Continue walking toward current direction
            vel.x = (facingRight ? 1.f : -1.f) * walkSpeed;
            setState(EnemyState::Walk);
        }
        physicsAndAnim(dt);
    }
};

// ════════════════════════════════════════════════════════════════
//  Charger — patrols slowly, but if it sees the player nearby
//  within visionRange, it sprints toward them.
//
//  Speeds and vision range are tunable per-instance so level 2
//  can spawn Chargers that are noticeably more aggressive than
//  level 1's.
// ════════════════════════════════════════════════════════════════
class Charger : public Enemy {
    float walkSpeed;
    float chargeSpeed;
    float visionRange;
    Player* target = nullptr;
public:
    Charger(const EnemyFrameSpec& spec, Vector2f start,
            float minX, float maxX,
            float walk = 80.f, float charge = 260.f, float vision = 400.f)
        : Enemy(spec, start, minX, maxX),
          walkSpeed(walk), chargeSpeed(charge), visionRange(vision) {}

    void setTarget(Player* p) { target = p; }

    void update(float dt) override {
        if (state == EnemyState::Death || state == EnemyState::Hurt) {
            physicsAndAnim(dt);
            return;
        }

        // Detect player on the same vertical band, within range
        if (target) {
            float dx = target->position().x - pos.x;
            float dy = abs(target->position().y - pos.y);
            if (dy < 100.f && abs(dx) < visionRange) {
                // Charge toward player
                facingRight = (dx > 0.f);
                vel.x = (facingRight ? 1.f : -1.f) * chargeSpeed;
                setState(EnemyState::Walk);
                physicsAndAnim(dt);
                return;
            }
        }
        // Idle patrol — small back-and-forth
        vel.x = (facingRight ? 1.f : -1.f) * walkSpeed;
        setState(EnemyState::Walk);
        physicsAndAnim(dt);
    }
};