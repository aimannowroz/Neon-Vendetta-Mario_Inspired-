#pragma once
#include "libraries.hpp"
#include "Globals.hpp"
#include "AssetCache.hpp"

// ════════════════════════════════════════════════════════════════
//  Tilemap — encodes the level as a rectangular grid of tile IDs.
//
//  Tile codes used in the layout strings:
//    '.'  empty (no draw, no collision)
//    '='  solid ground tile (collidable, draws ground variant)
//    '#'  solid platform (collidable, draws platform variant)
//    '-'  one-way platform top (player can jump up through, but
//         lands on top; falls off when descending)
//
//  Two display textures: the ground texture and the platform top
//  texture, picked from the per-level tileset.
// ════════════════════════════════════════════════════════════════
class Tilemap {
public:
    vector<string> rows;    // row 0 = TOP of level
    int width  = 0;
    int height = 0;

    // World-space pixel dimensions
    float worldW() const { return width  * G::PIXEL; }
    float worldH() const { return height * G::PIXEL; }

    // Load from in-code rows (pad shorter rows with '.')
    void load(const vector<string>& r) {
        rows = r;
        height = (int)rows.size();
        width  = 0;
        for (auto& row : rows) width = max(width, (int)row.size());
        for (auto& row : rows) row.resize(width, '.');
    }

    char at(int col, int row) const {
        if (row < 0 || row >= height || col < 0 || col >= width) return '.';
        return rows[row][col];
    }

    bool isSolid(char c)   const { return c == '=' || c == '#'; }
    bool isOneWay(char c)  const { return c == '-'; }
    bool isAnySolid(char c)const { return isSolid(c) || isOneWay(c); }

    // Convert a world point to tile cell. Assumes the map starts at
    // world (0, 0).
    Vector2i worldToTile(Vector2f p) const {
        return { (int)floor(p.x / G::PIXEL), (int)floor(p.y / G::PIXEL) };
    }

    // Tile cell to world rectangle
    FloatRect tileRect(int col, int row) const {
        return FloatRect({col * G::PIXEL, row * G::PIXEL},
                         {G::PIXEL, G::PIXEL});
    }
};
