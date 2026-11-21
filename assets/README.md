# Assets (not included)

The game's art, sound effects and pixel font come from free packs on [CraftPix.net](https://craftpix.net). Their licence allows using the files in a game, but not redistributing the raw files, so they aren't in this repository.

To run the game, download the cyberpunk character, enemy, tileset, background and GUI packs from CraftPix and place them in this folder using this layout:

```
assets/
  CyberpunkCraftpixPixel.otf
  music.mp3                      (optional, any track you have the rights to)
  characters/{biker,punk,cyborg}/
  enemies/{e1..e6}/
  bosses/{b1..b3}/
  levels/industrial/{tiles,objects,bg,animated}/
  levels/green/{tiles,objects,bg,animated}/
  gui/{frames,bars,icons,buttons,numbers,cursors,logos,decor}/
  sfx/  jump.wav coin.wav hurt.wav win.wav land.wav gameover.wav attack.wav hit.wav
```

The file names the code expects can be found in `Codes/AssetCache.hpp`, `Codes/Level.hpp` and the state files.
