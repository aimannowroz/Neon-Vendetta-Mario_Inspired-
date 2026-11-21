#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"

// ════════════════════════════════════════════════════════════════
//  Parallax — multi-layer scrolling background.
//  Each level has 5 PNG layers; the back ones move slowly (distant),
//  the front ones move almost in lockstep with the camera.
// ════════════════════════════════════════════════════════════════
class Parallax {
    static constexpr int   N = 5;
    static constexpr float FACTORS[N] = {0.05f, 0.15f, 0.30f, 0.55f, 0.80f};

    vector<Texture*> layers;     // borrowed from AssetCache
    float scaleX = 1.f, scaleY = 1.f;
    float layerW = 1.f;
    float camX   = 0.f;

public:
    void load(const string& folderPath, const string& filePrefix = "") {
        layers.clear();
        AssetCache& A = AssetCache::get();
        for (int i = 1; i <= N; i++) {
            string path = folderPath + "/" + filePrefix + to_string(i) + ".png";
            try {
                layers.push_back(&A.tex(path));
            } catch (...) {
                cerr << "Parallax: missing " << path << "\n";
                layers.push_back(nullptr);
            }
        }
        // Compute scale from first existing layer
        for (auto* t : layers) {
            if (t) {
                Vector2u s = t->getSize();
                scaleY = G::SCREEN_H / (float)s.y;
                scaleX = scaleY;
                layerW = (float)s.x * scaleX;
                break;
            }
        }
    }

    void setCamera(float x) { camX = x; }

    void render(RenderWindow& w) {
        const float ref = G::SCREEN_W / 2.f;
        for (int i = 0; i < N; i++) {
            if (!layers[i]) continue;
            float off = -(camX - ref) * FACTORS[i];
            float wrap = fmod(off, layerW);
            if (wrap > 0.f) wrap -= layerW;
            for (int tile = 0; tile < 3; tile++) {
                Sprite s(*layers[i]);
                s.setScale({scaleX, scaleY});
                s.setPosition({wrap + tile * layerW, 0.f});
                w.draw(s);
            }
        }
    }
};
