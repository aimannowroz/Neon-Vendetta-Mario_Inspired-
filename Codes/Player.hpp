#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AnimatedSprite.hpp"
#include "AssetCache.hpp"
#include "SoundManager.hpp"

// ════════════════════════════════════════════════════════════════
//  Player — the controllable character.
//
//  States: IDLE, RUN, JUMP, DJUMP, ATTACK, HURT, DEATH.
//  Uses one AnimatedSprite per state, switched in setState().
//
//  OOP CONCEPTS: Encapsulation (private state, public methods),
//  Composition (owns 7 AnimatedSprites + Vector2f physics).
//
//  Hitbox is calibrated tightly around the actual character pixels
//  inside the 48×48 frame: x[3-22], y[14-47] (per analysis of the
//  Biker spritesheet).
// ════════════════════════════════════════════════════════════════

enum class PlayerState { Idle, Run, Jump, DJump, Attack, Hurt, Death };

class Player {
    // Each animation is owned in a vector parallel to the enum.
    vector<AnimatedSprite> anims;

    // Per-state frame counts (Biker/Punk/Cyborg all share the exact
    // same animation layout — 4/6/4/6/6/2/6 frames per state).
    static constexpr int FRAMES_IDLE   = 4;
    static constexpr int FRAMES_RUN    = 6;
    static constexpr int FRAMES_JUMP   = 4;
    static constexpr int FRAMES_DJUMP  = 6;
    static constexpr int FRAMES_ATTACK = 6;   // attack1
    static constexpr int FRAMES_HURT   = 2;
    static constexpr int FRAMES_DEATH  = 6;

    PlayerState state    = PlayerState::Idle;
    Vector2f    pos      {0.f, 0.f};       // bottom-centre of character
    Vector2f    vel      {0.f, 0.f};
    bool        facingRight = true;
    bool        onGround = false;
    bool        canDoubleJump = false;
    bool        usedDoubleJump = false;

    // Combat
    int    hp           = G::PLAYER_MAX_HP;
    float  invulnTimer  = 0.f;     // counts down after a hit
    float  attackTimer  = 0.f;     // counts down between attacks
    bool   attackHitFlag = false;  // set true while the swing connects

    // QoL
    float coyoteTimer = 0.f;
    float jumpBuffer  = 0.f;

    // Hitbox is 18×33 px (unscaled) -> 63×116 px scaled
    static constexpr float HBOX_W = 18.f * G::PLAYER_SCALE;
    static constexpr float HBOX_H = 33.f * G::PLAYER_SCALE;

public:
    Player(const string& charDir) {
        AssetCache& A = AssetCache::get();

        // Derive the file prefix from the directory name. Char names match:
        //   ../assets/characters/biker  -> Biker
        //   ../assets/characters/punk   -> Punk
        //   ../assets/characters/cyborg -> Cyborg
        string prefix;
        size_t slash = charDir.find_last_of("/\\");
        string leaf = (slash == string::npos) ? charDir : charDir.substr(slash + 1);
        if (!leaf.empty()) {
            prefix = leaf;
            prefix[0] = toupper((unsigned char)prefix[0]);
        }

        auto P = [&](const string& anim) {
            return charDir + "/" + prefix + "_" + anim + ".png";
        };

        anims.emplace_back(A.tex(P("idle")),       FRAMES_IDLE,   0.12f);
        anims.emplace_back(A.tex(P("run")),        FRAMES_RUN,    0.08f);
        anims.emplace_back(A.tex(P("jump")),       FRAMES_JUMP,   0.10f, false);
        anims.emplace_back(A.tex(P("doublejump")), FRAMES_DJUMP,  0.07f, false);
        anims.emplace_back(A.tex(P("attack1")),    FRAMES_ATTACK, 0.07f, false);
        anims.emplace_back(A.tex(P("hurt")),       FRAMES_HURT,   0.10f, false);
        anims.emplace_back(A.tex(P("death")),      FRAMES_DEATH,  0.12f, false);

        for (auto& a : anims) a.setScale(G::PLAYER_SCALE);
    }

    // ── Public mutators / queries ──────────────────────────────
    void   setPosition(Vector2f p) { pos = p; vel = {0.f, 0.f}; }
    Vector2f position()       const { return pos; }
    Vector2f velocity()       const { return vel; }
    int      health()         const { return hp; }
    bool     isFacingRight()  const { return facingRight; }
    bool     isAttacking()    const { return state == PlayerState::Attack; }
    bool     isDead()         const { return state == PlayerState::Death && anims[6].finished; }
    bool     isInvulnerable() const { return invulnTimer > 0.f; }

    // Hitbox = box centred on bottom-centre origin
    FloatRect hitbox() const {
        return FloatRect({pos.x - HBOX_W / 2.f, pos.y - HBOX_H},
                         {HBOX_W, HBOX_H});
    }

    // Rectangle representing the area the player's currently swinging through
    FloatRect attackBox() const {
        if (state != PlayerState::Attack) return FloatRect({0,0}, {0,0});
        // Sword/punch reaches in the facing direction
        float halfW = G::ATTACK_REACH;
        float cx = pos.x + (facingRight ? halfW * 0.4f : -halfW * 0.4f);
        return FloatRect(
            {cx - halfW * 0.6f, pos.y - HBOX_H * 0.85f},
            {halfW * 1.2f, G::ATTACK_HALF_H * 2.f});
    }

    // Called by Level when the player lands a hit (so we don't re-hit
    // every frame for the duration of the swing).
    void   markAttackConsumed() { attackHitFlag = true; }
    bool   attackUnconsumed() const { return state == PlayerState::Attack && !attackHitFlag; }

    void reset(int startingHp = G::PLAYER_MAX_HP) {
        hp = startingHp;
        state = PlayerState::Idle;
        for (auto& a : anims) a.reset();
        vel = {0.f, 0.f};
        invulnTimer = attackTimer = 0.f;
        attackHitFlag = false;
        canDoubleJump = false;
        usedDoubleJump = false;
        coyoteTimer = jumpBuffer = 0.f;
    }

    void heal(int amount) {
        hp = min(G::PLAYER_MAX_HP, hp + amount);
    }

    // ── State management ─────────────────────────────────────────
    void setState(PlayerState s) {
        if (state == s) return;
        state = s;
        anims[(int)state].reset();
        if (s == PlayerState::Attack) attackHitFlag = false;
    }

    // ── Damage ───────────────────────────────────────────────────
    bool takeDamage(int dmg, float fromX) {
        if (invulnTimer > 0.f || state == PlayerState::Death) return false;
        hp -= dmg;
        invulnTimer = G::HURT_INVULN_TIME;
        // Knockback away from source
        float dir = (pos.x < fromX) ? -1.f : 1.f;
        vel.x = dir * G::KNOCKBACK_X;
        vel.y = G::KNOCKBACK_Y;
        onGround = false;
        if (hp <= 0) {
            hp = 0;
            setState(PlayerState::Death);
        } else {
            setState(PlayerState::Hurt);
        }
        SoundManager::get().playHurt();
        return true;
    }

    // ── Per-frame input handler (called from Level::update) ──────
    void handleInput() {
        if (state == PlayerState::Death) return;

        bool left  = Keyboard::isKeyPressed(Keyboard::Key::A) ||
                     Keyboard::isKeyPressed(Keyboard::Key::Left);
        bool right = Keyboard::isKeyPressed(Keyboard::Key::D) ||
                     Keyboard::isKeyPressed(Keyboard::Key::Right);

        // Lock movement during attack/hurt so animations finish cleanly
        bool actionLock = (state == PlayerState::Attack ||
                           state == PlayerState::Hurt);

        if (!actionLock) {
            if (left)  { vel.x = -G::MOVE_SPEED; facingRight = false; }
            else if (right) { vel.x =  G::MOVE_SPEED; facingRight = true; }
            else vel.x = 0.f;
        }
    }

    // Called once per event-loop pass (for jump-press-edge detection)
    void onJumpPressed() {
        if (state == PlayerState::Death || state == PlayerState::Hurt) return;
        jumpBuffer = G::JUMP_BUFFER;
    }

    void onAttackPressed() {
        if (state == PlayerState::Death || state == PlayerState::Hurt) return;
        if (attackTimer <= 0.f) {
            setState(PlayerState::Attack);
            attackTimer = G::ATTACK_COOLDOWN;
            SoundManager::get().playAttack();
        }
    }

    // ── Per-frame physics & animation update ─────────────────────
    void update(float dt) {
        // Timers
        if (invulnTimer > 0.f) invulnTimer -= dt;
        if (attackTimer > 0.f) attackTimer -= dt;
        if (coyoteTimer > 0.f) coyoteTimer -= dt;
        if (jumpBuffer  > 0.f) jumpBuffer  -= dt;

        // Coyote-time grace
        if (onGround) coyoteTimer = G::COYOTE_TIME;

        // Jump consumption (buffered or just pressed)
        if (jumpBuffer > 0.f && state != PlayerState::Death) {
            if (onGround || coyoteTimer > 0.f) {
                vel.y = G::JUMP_VEL;
                onGround = false;
                jumpBuffer = 0.f;
                coyoteTimer = 0.f;
                usedDoubleJump = false;
                canDoubleJump = true;
                setState(PlayerState::Jump);
                SoundManager::get().playJump();
            } else if (canDoubleJump && !usedDoubleJump) {
                vel.y = G::DJUMP_VEL;
                jumpBuffer = 0.f;
                usedDoubleJump = true;
                setState(PlayerState::DJump);
                SoundManager::get().playJump();
            }
        }

        // Gravity
        vel.y += G::GRAVITY * dt;
        if (vel.y > 1500.f) vel.y = 1500.f;

        // Position integration is left to Level::update so it can apply
        // collision against tilemap. Player::applyMovement does the actual
        // shifting once Level has resolved horizontal & vertical phases.

        // ── Update animation choice based on state/physics ──
        if (state == PlayerState::Death) {
            // stay in death anim
        } else if (state == PlayerState::Hurt) {
            if (anims[5].finished) setState(PlayerState::Idle);
        } else if (state == PlayerState::Attack) {
            if (anims[4].finished) {
                attackTimer = G::ATTACK_COOLDOWN;
                setState(onGround ? (abs(vel.x) > 1.f ? PlayerState::Run
                                                       : PlayerState::Idle)
                                  : PlayerState::Jump);
            }
        } else if (state == PlayerState::DJump) {
            if (anims[3].finished || onGround) {
                if (onGround) setState(abs(vel.x) > 1.f ? PlayerState::Run
                                                        : PlayerState::Idle);
                else          setState(PlayerState::Jump);
            }
        } else if (!onGround) {
            if (state != PlayerState::Jump && state != PlayerState::DJump)
                setState(PlayerState::Jump);
        } else {
            if (abs(vel.x) > 1.f) setState(PlayerState::Run);
            else                   setState(PlayerState::Idle);
        }

        // Advance current animation
        anims[(int)state].update(dt);

        // Position the active animation at the player position
        anims[(int)state].setPosition(pos);
        anims[(int)state].setScale(G::PLAYER_SCALE, !facingRight);
    }

    // Level applies horizontal motion in two phases:
    void moveX(float dx) { pos.x += dx; }
    void moveY(float dy) { pos.y += dy; }
    void   setOnGround(bool b) {
        bool wasInAir = !onGround;
        onGround = b;
        if (b && wasInAir && state != PlayerState::Death) {
            // Just landed — sound only if falling fast
            if (vel.y > 200.f) SoundManager::get().playLand();
            vel.y = 0.f;
            usedDoubleJump = false;
            canDoubleJump = false;
        }
    }
    Vector2f& mutableVel() { return vel; }   // Level needs to zero vel on collision

    // ── Render ───────────────────────────────────────────────────
    void render(RenderWindow& w) {
        // i-frame flicker — alternate visibility while invulnerable
        if (invulnTimer > 0.f) {
            int phase = (int)(invulnTimer * 12.f) % 2;
            if (phase == 0) anims[(int)state].draw(w);
        } else {
            anims[(int)state].draw(w);
        }
    }
};
