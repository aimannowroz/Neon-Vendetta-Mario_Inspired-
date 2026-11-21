# Neon Vendetta

A cyberpunk action platformer written in C++17 with SFML 3. It was built as an end-of-semester Object-Oriented Programming project at GIK Institute by a team of three.

Three rebels, the Biker, the Punk and the Cyborg, have to fight their way through the Industrial Zone and the Green Zone to escape a corporate megacity. Pick a fighter, beat the clock and reach the portal.

**Team:** Aiman Nowrooz, _teammate names to be added_

## Features

- Three playable characters, each with a full set of animations
- Two levels with five-layer parallax backgrounds
- Two enemy types with different AI: Walkers patrol, Chargers rush you
- Coins, keycards, health pickups and chests
- Double jump, melee attack, a level timer, a HUD with hearts and score
- A saved high score with a "NEW RECORD" banner
- Menu, character select, pause, win and game-over screens

## Controls

| Key | Action |
|---|---|
| A / D or ← / → | Move |
| W or ↑ | Jump (press again in the air to double-jump) |
| J | Attack |
| Esc | Pause |
| Space | Resume |
| Q | Quit to menu (from pause) |
| Enter | Confirm / continue / retry |

**Scoring:** coin 10 · enemy 25 · keycard 50 · chest 100 · +5 for every second left on the clock when you finish a level.

## OOP concepts demonstrated

- **Three inheritance hierarchies**
  - `GameState` → Menu, CharSelect, Level, Pause, Win, GameOver
  - `Entity` → Coin, Card, HealthPickup, Chest
  - `Enemy` → Walker, Charger
- **Polymorphism** through `vector<unique_ptr<Entity>>` and `vector<unique_ptr<Enemy>>`, with `dynamic_cast` for pickup-specific behaviour
- **Singletons:** `AssetCache` (loads each texture and font once) and `SoundManager`
- **Composition:** `Player` owns its `AnimatedSprite`s; `Level` owns a `Tilemap`, `Parallax` and `HUD`
- **Encapsulation**, **file I/O** for the high score, and **strategy-style AI** through overridden `update()` methods
- A **state machine** in `main.cpp` drives screen transitions

## Build and run

You need **SFML 3.0+** and a C++17 compiler. The `assets` folder must stay next to the `Codes` folder, and the game must be run from inside `Codes/`.

**Windows (MinGW):**

```bash
cd Codes
g++ main.cpp -o app -std=c++17 -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
app.exe
```

**macOS (Homebrew):**

```bash
brew install sfml
cd Codes
clang++ main.cpp -o app -std=c++17 -I"$(brew --prefix sfml)/include" -L"$(brew --prefix sfml)/lib" \
  -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
./app
```

**Art and sound:** the CraftPix asset files aren't included in this repo, because their licence doesn't allow redistributing them. See `assets/README.md` for how to set them up.

**Music:** the original background track isn't included for copyright reasons. To add music, put any MP3 you have the rights to at `assets/music.mp3`. Without one, the game runs silently apart from sound effects.

## Project layout

```
Codes/
  main.cpp              state machine and game loop
  GameState.hpp         abstract base for every screen
  MenuState.hpp, CharSelectState.hpp, LevelState.hpp,
  PauseState.hpp, WinState.hpp, GameOverState.hpp
  Player.hpp            controllable character
  Enemy.hpp             Walker / Charger
  Entity.hpp            pickups and chests
  Level.hpp             tilemap, entities, camera, collision
  Tilemap.hpp, Parallax.hpp, HUD.hpp, AnimatedSprite.hpp
  AssetCache.hpp, SoundManager.hpp, BestScore.hpp
  Globals.hpp           tunable constants
assets/                 sprites, tiles, UI, sound effects, pixel font
```

## Credits

Art, sound effects and the pixel font are from [CraftPix.net](https://craftpix.net), used under their free licence. The files themselves are not redistributed here.

### SFML 3 notes, for anyone porting from SFML 2

- `Sprite` has no default constructor, so always pass a `Texture&`.
- `Texture(path)` throws `sf::Exception` on failure.
- `sf::Sound` needs a `SoundBuffer&` at construction. This project uses `optional<Sound>`.
- Events use `optional<Event>`: `ev->is<Event::Closed>()` and `ev->getIf<Event::KeyPressed>()`.
- Use `Music::setLooping(true)`, not `setLoop`.
- `sf::Uint8` is gone; use `uint8_t`.
- `IntRect` and `FloatRect` use `{position, size}`.
