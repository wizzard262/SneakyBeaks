/*
 * Sneaky Beaks - constants, animation tables and sprite fixes.
 * Names in comments (time_max, skittle_max, ...) are the #defines in BEK_SCR8.C.
 */
(function (SB) {
  'use strict';

  SB.config = {
    SCREEN_W: 320, SCREEN_H: 200,   // VGA mode 13h, like the original
    TICK_MS: 70,                    // original main loop: delay(70)

    START_BEAKS: 3,                 // num_beaks
    MAX_BEAKS: 8,
    MAX_COINS: 20,                  // coins allowed on the field at once
    HUNGER_MAX: 520,                // time_max: ticks a duck lives without food (~36 s)
    SUGAR_TIME: 45,                 // skittle_max: ticks of "just ate a sweet" happiness
    BAG_MAX: 6,                     // bag_max: sweets in a bag from the shop
    KICK_COOLDOWN: 18,              // ticks before a kicker can kick again
    KICK_IMMUNE: 22,                // ticks a kicked duck can't be kicked again
    EVADE_RADIUS: 60,               // radius: how close a hungry duck may get to a bag duck
    GRAVITY: 1,
    COIN_FALL: 8,                   // gravity in the original: dropped coins fall 8px per tick

    // where a duck's feet may go
    MIN_X: 12, MAX_X: 308, MIN_Y: 36, MAX_Y: 196,
    // duck sprites are drawn with their feet at (x, y)
    DUCK_OX: 11, DUCK_OY: 31,
    // shop: top-left of its 64x64 block (original shop_x 150, shop_y 90)
    SHOP_X: 150, SHOP_Y: 70,
  };

  // Frame numbers in BEAK.SPR. Walk/coin/eat/dying tables come from BEK_SCR8.C;
  // the kick/tumble/peck ones were placeholders there and were picked from the art.
  SB.ANIM = {
    walk:  { n: [2, 3],   e: [9, 10, 11, 12, 13, 12, 11, 10],  s: [0, 1],   w: [4, 5, 6, 7, 8, 7, 6, 5] },
    walkM: { n: [25, 26], e: [32, 33, 34, 35, 36, 35, 34, 33], s: [23, 24], w: [27, 28, 29, 30, 31, 30, 29, 28] },
    walkB: { n: [39, 40], e: [46, 47, 48, 49, 50, 49, 48, 47], s: [37, 38], w: [41, 42, 43, 44, 45, 44, 43, 42] },
    coinFlip: [14, 15, 16, 17, 16, 15],
    coin: 14,
    sweets: [18, 19, 20, 21, 22],                   // red, orange, yellow, green, purple
    flapping: [51, 52, 53, 52],
    looking: [54, 54, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 60, 59, 59, 58, 58, 57, 57, 56, 56, 55, 55],
    peckW: [61, 62, 63, 63, 62, 61],      peckE: [64, 65, 66, 66, 65, 64],
    peckCoinW: [61, 62, 63, 67, 67, 62],  peckCoinE: [64, 65, 66, 68, 68, 65],
    kickW: [69, 69, 70, 70, 70, 70, 69, 69, 61, 61, 61],
    kickE: [78, 78, 79, 80, 80, 80, 79, 79, 64, 64, 64],
    tumbleW: [71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76],
    tumbleE: [81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86],
    eating: [                                       // eating1/2/3: purple, red, green
      [88, 89, 90, 91, 92, 93, 94, 95, 96],
      [88, 89, 90, 97, 98, 93, 94, 95, 96],
      [88, 89, 90, 99, 100, 93, 94, 95, 96],
    ],
    dying: [104, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114],
    shop: [87, 101, 102, 103],
  };

  SB.NAMES  = ['Quackers', 'Bill', 'Puddles', 'Waddles', 'Nugget', 'Dot', 'Sneaky', 'Beaky', 'Pip', 'Mallory', 'Drake', 'Fluff'];
  SB.COLORS = ['#ff5a5a', '#5ab4ff', '#6bef6b', '#ffb02e', '#ff6bff', '#5affff', '#ffffff', '#b48cff'];

  // Fixes to glitches in the original art (BEAK.SPR itself is left untouched).
  SB.patchSprites = function (gfx) {
    // Flap frame 51: the raised wings were painted in colour 251 (almost black)
    // instead of the wing yellow (75) used by frames 52/53, and the lowered arms
    // from the standing pose were left in below the shoulders.
    const s = gfx.sprites[51];
    if (s && s.w === 26 && s.h === 32) {
      const px = s.raw.pixels;
      for (let i = 0; i < px.length; i++) if (px[i] === 251) px[i] = 75;
      for (let y = 12; y < 18; y++)
        for (const x of [6, 7, 18, 19]) if (px[y * s.w + x] === 75) px[y * s.w + x] = 0;
      s.canvas = WGT.toCanvas(s.raw, gfx.palette);
      s.bbox = WGT.bbox(s.raw);
    }
    // only animate the shop if its extra frames exist and match frame 87
    SB.ANIM.shop = SB.ANIM.shop.filter(n => gfx.sprites[n] && gfx.sprites[n].w === gfx.sprites[87].w);
  };
})(window.SB = window.SB || {});
