# Sneaky Beaks

A DOS game written in C in 1998, brought back to life in the browser.

Ducks wander around a field. You drop a coin. The ducks race for it, and the winner carries it to the shop to buy a bag of sweets. Hungry ducks chase any duck holding a coin or a bag, or one that has just got a sweet, and kick it. That can make it drop the coin, spill sweets from the bag, or lose the sweet right out of its mouth. Ducks with sweets try to keep away from the others. A duck that goes too long without eating a sweet dies.

## What we are doing

The original game was written in 1998 for DOS in C with the **WordUp Graphics Toolkit 4.5** (WGT). It used VGA mode 13h (320×200, 256 colours) and loaded its graphics from `BEAK.SPR`, a WGT sprite file that modern tools can't open.

This project:

1. **Archives the original code** in [`archive/`](archive/), unchanged.
2. **Reads `BEAK.SPR` directly in the browser.** [`js/spr.js`](js/spr.js) is a JavaScript port of WGT's `wloadsprites()` from the WGT95 source (`LOADSPR.C`). All 115 original sprites are rendered from the original file. No sprites were redrawn or converted.
3. **Ports the game to JavaScript and an HTML `<canvas>`.** [`js/game.js`](js/game.js) keeps the C program's structure: the same beak states (`noskittle`, `skittle`, `money`, `bag`, `pecking`, `kicking`, `kicked`, `dying`, `dead`), the same move states (random, track, evade, looking, flapping, gobble), the same animation frame tables, and the same 320×200 screen and 70 ms tick (`delay(70)`). Parts the C version never finished are filled in to match how the game was meant to play: dropping the coin or sweets when kicked, eating from the bag, and starving.
4. **Runs fully client-side, with no build step and no server code.** It can be hosted on GitHub Pages, or you can just double-click `index.html`.

Why JavaScript and canvas: they run everywhere, need no install or toolchain, are easy to host as static files, and a 320×200 indexed-colour sprite game maps naturally onto a canvas scaled up with crisp pixels.

## Play

- **Online:** see *Hosting on GitHub Pages* below.
- **Locally:** open `index.html` in any modern browser. It works from `file://` because the sprite file is also embedded in `assets/beak.spr.js`.

| Key | Action |
| --- | --- |
| `Space` / `M` | Drop a coin at a random spot (`M` was the key in the original) |
| Click / tap | Drop a coin exactly there |
| `N` | A new duck arrives (up to 8) |
| `P` | Pause |
| `F` | Toggle 2× speed |
| `R` | Restart |
| `H` | Show or hide the hunger bars |
| `B` | Grass or the original black background |
| `S` | Sound on or off |

## Sprite viewer

[`sprites.html`](sprites.html) shows every sprite in `BEAK.SPR` with its index, size, what the game uses it for, and the 256-colour palette. You can click a sprite to save it as a PNG, download the whole sheet, or open any other WGT `.SPR` file (versions 1 to 4).

### The `.SPR` format (WGT 4.x)

All values are little-endian.

| Field | Size | Notes |
| --- | --- | --- |
| version | u16 | `<= 3`: sprites are numbered from 1. `4`: numbered from 0. `BEAK.SPR` is version 4. |
| magic | 13 bytes | `" Sprite File "` |
| palette | 768 bytes | 256 × RGB, 6-bit VGA values (0 to 63) |
| maxsprite | i16 | Highest sprite number in the file (114 in `BEAK.SPR`) |
| *each sprite* | | u16 `made` flag. If it is 1: u16 width, u16 height, then width×height bytes of palette indexes, row by row. Index 0 is transparent. |

## Project layout

```
index.html          the game
sprites.html        WGT sprite viewer / exporter
css/style.css
js/spr.js           .SPR reader (port of WGT wloadsprites)
js/config.js        constants, animation frame tables, fixes to glitches in the art
js/sound.js         little WebAudio sound effects
js/game.js          the simulation: duck states and AI, coins and sweets (port of BEK_SCR8.C / BEK_SCR.C)
js/render.js        drawing the field on the canvas, and the HUD
js/main.js          start-up, keyboard/mouse input, main loop
assets/BEAK.SPR     original sprite file
assets/beak.spr.js  BEAK.SPR embedded as base64 (generated)
tools/embed-spr.js  regenerates assets/beak.spr.js
archive/            the original DOS files, untouched
```

If you edit `BEAK.SPR`, regenerate the embedded copy:

```
node tools/embed-spr.js assets/BEAK.SPR assets/beak.spr.js
```

The port started from `BEK_SCR8.C` (September 1998). These rules were later brought in from the final `BEK_SCR.C` (February 1999):

- **Happy ducks can be kicked.** Ducks that have just eaten a sweet are targets too. A kick has a 50% chance of knocking the sweet out of the duck's mouth.
- **Kicks scatter loot.** A kicked bag throws out 0–4 sweets, and a dropped coin can fly a long way.
- **Happy ducks are calmer.** They mostly look around. When one flaps, it takes off up the screen.
- **The 1999 AI weights.** Hungry ducks go for sweets on the floor before coins and leave them alone a third of the time. With nothing on the floor, they always hunt the nearest duck worth kicking. A coin carrier dawdles 1 time in 4 on the way to the shop.

Ways the port tightens up the original's behaviour:

- **The shop is solid.** Ducks route round it by the shortest path via its corners. Coins and sweets never land on it, so nothing gets hidden or stuck behind it.
- **Ducks with sweets don't get cornered.** They escape by scoring all 8 directions for distance from the hungry ducks, with a penalty for being near walls and corners.
- **Ducks don't stack.** Ducks walking into each other are nudged apart instead of merging into one sprite.
- **Stuck ducks recover.** A duck that can't make progress towards its target wanders off for a moment and then tries again.
- **Peck, kick, tumble and eat end with their animations.** No separate timers that can drift out of step.
- **Starving ducks eat first.** A duck that is nearly starving goes for sweets before coins. A duck with a bag eats even while being chased. A duck that dies drops whatever it was carrying.
- **Coins are capped.** At most 20 on the field at once.

For debugging, `index.html?sim=2000&coins=60` fast-forwards 2000 ticks before the game starts and drops a coin every 60 ticks. Use `coins=0` to watch them starve.

## Hosting on GitHub Pages

1. Create a GitHub repository and push this folder to it as the repository root.
2. In the repository, go to **Settings → Pages → Build and deployment**. Set **Source** to *Deploy from a branch*, choose `main` and `/ (root)`, then save.
3. After a minute the game is live at `https://<user>.github.io/<repo>/`.

All paths are relative and there is no build step, so it also works under a sub-path or on any static host.

## The archive

| File | What it is |
| --- | --- |
| `BEK_SCR.C` | **Final version** (14 Feb 1999). `20.EXE` was compiled from it 10 seconds later. |
| `BEK_SCR8.C` | September 1998 version; the port was first based on this |
| `BEAK_SCR.C`, `BEK_SCR2.C`, `BEK_SCR5-7.C` | Earlier versions (July–August 1998) |
| `WGT45.C`, `WGT45.H` | WordUp Graphics Toolkit 4.5 header and example program |
| `BEAK.SPR` | Sprite file (115 sprites + palette) |
| `20.EXE` | Compiled DOS build of `BEK_SCR.C` (it contains the "SNEAKY BEAKS" title text and loads `beak.spr`) |
| `SPR5.EXE` | Old DOS executable (probably the WGT sprite editor) |
| `*.ppt` | Design PowerPoints (state machine diagrams). Not used by the port. |

The WGT95 source used as the reference for the `.SPR` reader was released under the GNU LGPL by its author, Chris Egerter.

---

## Original prompt

> look in C:\DEV\Other\DuckStuff
> It was a game i made in C in the past the file:
> its simple, ducks walk around the screen, you press space to drop a coin,
> the ducks race to get the coin and take it to the shop to buy sweets
> ducks chase others with a coin or a bag of sweets to kick him and he may drop the coin or some sweets
> ducks with sweets try to avoid other ducks
> if any duck does not eat sweets for too long he dies
>  * beak.spr uses some old format i cannot find an opener for now
> * the ppt are just powerpoints ignore
> * the latest code verison is bek_scr8.c
> * WGT45.C & WGT45.h are libraries it sues
> * This folder C:\DEV\Other\DuckSpriteGameResources\wgt95_sc
>   has source for soemthing to read the sprite
>
> I want the game to work again rendering the sprites in whatefer client side only
> webpage mechanism is best (maybe javascript and canvas? you make the best choice)
> in fact is you can make it hostable on a github page
>
> I want you to create a new folder, put an archive folder in it with the old code
> and the new code in the root of the folder.
> put a README.MD in the root and explain what we are doing and put this prompt in it at the bottom
