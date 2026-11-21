#pragma once
#include "libraries.hpp"

// ════════════════════════════════════════════════════════════════
//  AssetCache  —  load a texture once, share the reference forever.
//
//  SFML 3 disallows a default-constructed Texture, so every Texture
//  in the cache is created via emplace + loadFromFile in a single
//  step. The cache returns a reference; callers store references
//  or pointers into the cache (the cache is alive for the whole
//  program lifetime).
//
//  OOP CONCEPT: Singleton — one cache for the whole program.
// ════════════════════════════════════════════════════════════════
class AssetCache {
    // unique_ptr lets us return stable references — std::unordered_map
    // would invalidate references on rehash, so we manage memory ourselves.
    vector<pair<string, unique_ptr<Texture>>> textures;
    vector<pair<string, unique_ptr<Font>>>    fonts;

    AssetCache() = default;

public:
    AssetCache(const AssetCache&)            = delete;
    AssetCache& operator=(const AssetCache&) = delete;

    static AssetCache& get() {
        static AssetCache inst;
        return inst;
    }

    // Returns reference to cached Texture. If first time seen, loads it.
    Texture& tex(const string& path) {
        for (auto& e : textures)
            if (e.first == path) return *e.second;
        // SFML 3: Texture(path) throws sf::Exception on failure
        auto p = make_unique<Texture>(path);
        textures.emplace_back(path, std::move(p));
        return *textures.back().second;
    }

    Font& font(const string& path) {
        for (auto& e : fonts)
            if (e.first == path) return *e.second;
        auto p = make_unique<Font>(path);
        fonts.emplace_back(path, std::move(p));
        return *fonts.back().second;
    }
};
