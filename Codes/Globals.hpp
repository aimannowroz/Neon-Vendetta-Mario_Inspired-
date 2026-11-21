#pragma once
#include "libraries.hpp"
//  Global constants — single point of tuning for the whole game
namespace G {

constexpr float SCREEN_W = 1920.f;
constexpr float SCREEN_H = 1080.f;

constexpr float TILE      = 32.f;        // raw tile pixel size
constexpr float TILE_SCALE = 3.f;        // we draw tiles at 3× -> 96 px on screen
constexpr float PIXEL     = TILE * TILE_SCALE;   // 96

constexpr float PLAYER_SCALE = 3.5f;     // 48 px frame * 3.5 = 168 px on screen

constexpr float GRAVITY    = 2200.f;
constexpr float MOVE_SPEED = 360.f;
constexpr float JUMP_VEL   = -1050.f;    // first jump
constexpr float DJUMP_VEL  = -900.f;     // double jump (slightly weaker)

constexpr float MAX_DT = 1.f / 30.f;     // cap dt to avoid physics blow-ups

// Combat
constexpr float ATTACK_COOLDOWN  = 0.45f;
constexpr float ATTACK_REACH     = 110.f;   // horizontal distance from player centre
constexpr float ATTACK_HALF_H    = 80.f;
constexpr int   PLAYER_MAX_HP    = 5;
constexpr float HURT_INVULN_TIME = 1.0f;
constexpr float KNOCKBACK_X      = 320.f;
constexpr float KNOCKBACK_Y      = -350.f;

// QoL feel
constexpr float COYOTE_TIME = 0.10f;
constexpr float JUMP_BUFFER = 0.10f;

}   // namespace G

// Random helper
inline float randf(float a, float b) {
    static mt19937 rng(random_device{}());
    return uniform_real_distribution<float>(a, b)(rng);
}
