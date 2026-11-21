#pragma once
#include "libraries.hpp"

// ════════════════════════════════════════════════════════════════
//  AnimatedSprite
//  Wraps an sf::Sprite + an arithmetic index that walks across a
//  horizontal-strip spritesheet. Construction takes a Texture
//  reference (SFML 3 requires that). Frame size is auto-derived
//  from texture dimensions: frame_h == texture_h, frame_w =
//  texture_w / frameCount.
//
//  OOP CONCEPT: Composition — Player and every Enemy *has-a*
//  vector<AnimatedSprite>, one per animation state.
// ════════════════════════════════════════════════════════════════
class AnimatedSprite {
public:
    Sprite  sprite;
    int     frameW       = 0;
    int     frameH       = 0;
    int     frameCount   = 1;
    int     currentFrame = 0;
    float   frameTime    = 0.10f;
    float   timer        = 0.f;
    bool    looping      = true;
    bool    finished     = false;

    AnimatedSprite(const Texture& tex, int frames, float frameTimeSec = 0.10f,
                   bool loop = true)
        : sprite(tex),
          frameH((int)tex.getSize().y),
          frameCount(frames),
          frameTime(frameTimeSec),
          looping(loop)
    {
        frameW = (int)tex.getSize().x / max(1, frames);
        sprite.setTextureRect(IntRect({0, 0}, {frameW, frameH}));
        // Origin = bottom-centre of frame so positioning is intuitive
        sprite.setOrigin({frameW / 2.f, (float)frameH});
    }

    void setScale(float s, bool flipped = false) {
        sprite.setScale({flipped ? -s : s, s});
    }

    void setPosition(Vector2f p) { sprite.setPosition(p); }

    void setFrameTime(float t) { frameTime = t; }

    void reset() {
        currentFrame = 0;
        timer = 0.f;
        finished = false;
        sprite.setTextureRect(IntRect({0, 0}, {frameW, frameH}));
    }

    void update(float dt) {
        if (finished && !looping) return;
        timer += dt;
        if (timer >= frameTime) {
            timer -= frameTime;
            currentFrame++;
            if (currentFrame >= frameCount) {
                if (looping) currentFrame = 0;
                else { currentFrame = frameCount - 1; finished = true; }
            }
            sprite.setTextureRect(IntRect({currentFrame * frameW, 0},
                                          {frameW, frameH}));
        }
    }

    void draw(RenderWindow& w) const { w.draw(sprite); }
};
