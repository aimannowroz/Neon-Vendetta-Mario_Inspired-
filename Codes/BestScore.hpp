#pragma once
#include "libraries.hpp"

// ════════════════════════════════════════════════════════════════
//  BestScore — static utility for high-score persistence.
//  Saves to "bestscore.txt" in the current working directory.
// ════════════════════════════════════════════════════════════════
namespace BestScore {

inline const char* PATH = "bestscore.txt";

inline int load() {
    ifstream f(PATH);
    if (!f) return 0;
    int v = 0;
    f >> v;
    return v;
}

// Returns true if score is a new high score; saves it if so.
inline bool maybeSave(int score) {
    int prev = load();
    if (score > prev) {
        ofstream f(PATH);
        if (f) f << score;
        return true;
    }
    return false;
}

}   // namespace BestScore
