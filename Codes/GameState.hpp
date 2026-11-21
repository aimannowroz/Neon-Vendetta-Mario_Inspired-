#pragma once
#include "libraries.hpp"

//  GameState — abstract base for every screen.
//  Polymorphism root- main() holds a GameState*
class GameState {
public:
    // outgoing-transition flag interface
    virtual ~GameState() = default;
    virtual void handleEvents(const optional<Event>& ev,
                              RenderWindow& window) = 0;
    virtual void update(float dt)                   = 0;
    virtual void render(RenderWindow& window)       = 0;
    virtual void onEnter() {}    // called when state becomes active
    virtual void onExit () {}    // called when leaving
};
