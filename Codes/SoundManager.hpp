#pragma once
#include "libraries.hpp"

// ════════════════════════════════════════════════════════════════
//  SoundManager — Singleton pattern.
//  Loads every SFX once and provides simple play methods.
//  All Sound objects use optional<> because SFML 3 requires a
//  SoundBuffer reference at Sound construction time.
// ════════════════════════════════════════════════════════════════
class SoundManager {
    SoundBuffer jumpBuf, coinBuf, hurtBuf, winBuf, landBuf, gameOverBuf;
    SoundBuffer attackBuf, hitBuf;

    optional<Sound> jumpS, coinS, hurtS, winS, landS, gameOverS, attackS, hitS;
    optional<Music> music;

    SoundManager() {
        load(jumpBuf,     "../assets/sfx/jump.wav",     jumpS,     55.f);
        load(coinBuf,     "../assets/sfx/coin.wav",     coinS,     70.f);
        load(hurtBuf,     "../assets/sfx/hurt.wav",     hurtS,     70.f);
        load(winBuf,      "../assets/sfx/win.wav",      winS,      80.f);
        load(landBuf,     "../assets/sfx/land.wav",     landS,     35.f);
        load(gameOverBuf, "../assets/sfx/gameover.wav", gameOverS, 75.f);
        load(attackBuf,   "../assets/sfx/attack.wav",   attackS,   55.f);
        load(hitBuf,      "../assets/sfx/hit.wav",      hitS,      65.f);

        music.emplace();
        if (music->openFromFile("../assets/music.mp3")) {
            music->setLooping(true);
            music->setVolume(35.f);
        } else {
            music.reset();
            cerr << "Note: no background music found (add your own track at assets/music.mp3)\n";
        }
    }

    static void load(SoundBuffer& buf, const string& path,
                     optional<Sound>& slot, float vol) {
        if (buf.loadFromFile(path)) {
            slot.emplace(buf);
            slot->setVolume(vol);
        } else {
            cerr << "Warning: SFX " << path << " missing\n";
        }
    }

public:
    SoundManager(const SoundManager&)            = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    static SoundManager& get() {
        static SoundManager inst;
        return inst;
    }

    void playJump()     { if (jumpS    ) jumpS    ->play(); }
    void playCoin()     { if (coinS    ) coinS    ->play(); }
    void playHurt()     { if (hurtS    ) hurtS    ->play(); }
    void playWin ()     { if (winS     ) winS     ->play(); }
    void playLand()     { if (landS    ) landS    ->play(); }
    void playGameOver() { if (gameOverS) gameOverS->play(); }
    void playAttack()   { if (attackS  ) attackS  ->play(); }
    void playHit()      { if (hitS     ) hitS     ->play(); }

    void startMusic() { if (music && music->getStatus() != Music::Status::Playing) music->play(); }
    void stopMusic()  { if (music) music->stop(); }
    void pauseMusic() { if (music) music->pause(); }

    void setMusicVolume(float v) { if (music) music->setVolume(v); }
};
