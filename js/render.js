/* Sneaky Beaks - drawing the field (canvas) and the HUD (DOM). */
(function (SB) {
  'use strict';

  const C = SB.config;
  const { SCREEN_W, SCREEN_H, DUCK_OX, DUCK_OY, HUNGER_MAX } = C;
  const clamp = (v, a, b) => v < a ? a : v > b ? b : v;
  const hungerColor = f => f > 0.5 ? '#5aff5a' : f > 0.25 ? '#ffd23c' : '#ff3c3c';

  // tiny 3x5 pixel font for floating text
  const FONT = {
    A: '010101111101101', B: '110101110101110', C: '011100100100011', E: '111100110100111', G: '011100101101011',
    H: '101101111101101', I: '111010010010111', K: '101101110101101', M: '101111111101101', N: '110101101101101',
    P: '110101110100100', S: '011100010001110', U: '101101101101111', Y: '101101010010010',
    '!': '010010010000010', '-': '000000111000000', ' ': '000000000000000',
  };

  function makeBackground(shop, classic) {
    const c = document.createElement('canvas');
    c.width = SCREEN_W; c.height = SCREEN_H;
    const g = c.getContext('2d');
    if (classic) { g.fillStyle = '#000'; g.fillRect(0, 0, SCREEN_W, SCREEN_H); return c; }
    // pixel grass (seeded, so it looks the same every time)
    g.fillStyle = '#2f7a2f'; g.fillRect(0, 0, SCREEN_W, SCREEN_H);
    const shades = ['#2a6f2a', '#358535', '#3a8f3a', '#286628', '#43993f'];
    let seed = 1234;
    const r = n => { seed = (seed * 16807) % 2147483647; return seed % n; };
    for (let i = 0; i < 2600; i++) { g.fillStyle = shades[r(shades.length)]; g.fillRect(r(SCREEN_W), r(SCREEN_H), 1, 1 + r(2)); }
    for (let i = 0; i < 40; i++) { g.fillStyle = ['#ffffff', '#ffe14d', '#ff9de0'][r(3)]; g.fillRect(r(SCREEN_W), r(SCREEN_H), 1, 1); }
    // a dirt path to the shop door
    g.fillStyle = 'rgba(120, 90, 50, 0.35)';
    g.fillRect(shop.doorX - 6, shop.doorY - 2, 12, SCREEN_H - shop.doorY + 2);
    return c;
  }

  function Renderer(canvas, gfx) {
    const ctx = canvas.getContext('2d');
    canvas.width = SCREEN_W; canvas.height = SCREEN_H;
    ctx.imageSmoothingEnabled = false;
    let bg = null;

    function drawSprite(n, x, y) {
      const s = gfx.sprites[n];
      if (s) ctx.drawImage(s.canvas, Math.round(x), Math.round(y));
    }
    // items sit in the corner of a 64x64 block: draw just the visible part, bottom-centred on (cx, cy)
    function drawItem(n, cx, cy) {
      const s = gfx.sprites[n];
      if (!s) return;
      const bb = s.bbox;
      ctx.drawImage(s.canvas, bb.x, bb.y, bb.w, bb.h, Math.round(cx - bb.w / 2), Math.round(cy - bb.h), bb.w, bb.h);
    }
    function pixelText(text, cx, y, color) {
      const x0 = Math.round(cx - (text.length * 4 - 1) / 2);
      for (const shadow of [1, 0]) {
        ctx.fillStyle = shadow ? '#000' : color;
        let px = x0;
        for (const ch of text) {
          const g = FONT[ch] || FONT[' '];
          for (let i = 0; i < 15; i++) if (g[i] === '1') ctx.fillRect(px + (i % 3) + shadow, y + Math.floor(i / 3) + shadow, 1, 1);
          px += 4;
        }
      }
    }

    return {
      setBackground(shop, classic) { bg = makeBackground(shop, classic); },

      draw(world, view) {
        const { shop, items, beaks, tick } = world;
        ctx.drawImage(bg, 0, 0);

        // shadows under airborne items
        ctx.fillStyle = 'rgba(0,0,0,0.35)';
        for (const it of items) if (it.z > 0) ctx.fillRect(Math.round(it.x) - 2, Math.round(it.y) - 1, 5, 1);

        // painter's algorithm: everything on the ground sorted by its feet
        const list = [{ y: shop.box.y1, draw: () => drawSprite(shop.frame, shop.x, shop.y) }];
        for (const it of items) if (it.z === 0) list.push({ y: it.y, draw: () => drawItem(it.frame, it.x, it.y) });
        for (const b of beaks) list.push({ y: b.y, draw: () => drawSprite(b.frame, b.x - DUCK_OX, b.y - DUCK_OY) });
        list.sort((a, b) => a.y - b.y);
        for (const d of list) d.draw();

        for (const it of items) if (it.z > 0) drawItem(it.frame, it.x, it.y - it.z);   // airborne items on top

        if (view.showBars) {                       // colour tag + hunger bar over each duck
          for (const b of beaks) {
            if (!SB.Game.alive(b)) continue;
            const bx = Math.round(b.x) - 8, by = Math.round(b.y) - DUCK_OY - 5;
            const f = clamp(b.hunger / HUNGER_MAX, 0, 1);
            ctx.fillStyle = '#000'; ctx.fillRect(bx - 4, by - 1, 22, 4);
            ctx.fillStyle = b.color; ctx.fillRect(bx - 3, by, 2, 2);
            ctx.fillStyle = '#333'; ctx.fillRect(bx, by, 16, 2);
            ctx.fillStyle = f <= 0.25 && (tick & 2) ? '#a01010' : hungerColor(f);
            ctx.fillRect(bx, by, Math.max(1, Math.round(16 * f)), 2);
          }
        }
        for (const f of world.floaters) pixelText(f.text, f.x, Math.round(f.y - f.t), f.color);

        if (view.paused) pixelText('PAUSE', SCREEN_W / 2, SCREEN_H / 2 - 3, '#ffffff');
        if (world.gameOver) {
          ctx.fillStyle = 'rgba(0,0,0,0.6)'; ctx.fillRect(0, 80, SCREEN_W, 40);
          ctx.textAlign = 'center';
          ctx.fillStyle = '#ffe14d'; ctx.font = 'bold 10px monospace';
          ctx.fillText('ALL THE DUCKS HAVE STARVED', SCREEN_W / 2, 97);
          ctx.fillStyle = '#fff'; ctx.font = '8px monospace';
          ctx.fillText('R = restart    N = new duck', SCREEN_W / 2, 111);
        }
      },
    };
  }

  // ------------------------------------------------------------------ HUD
  const STATE_TEXT = {
    noskittle: 'hungry', skittle: 'happy', money: 'has the coin', bag: 'has sweets',
    pecking: 'pecking', kicking: 'kicking!', kicked: 'ouch!', dying: 'dying', dead: 'dead',
  };

  function updateHud(world, view) {
    const $ = id => document.getElementById(id);
    $('roster').innerHTML = world.beaks.map(b => {
      const f = clamp(b.hunger / HUNGER_MAX, 0, 1);
      const st = b.state === 'bag' ? 'has ' + b.bag + ' sweet' + (b.bag === 1 ? '' : 's') : STATE_TEXT[b.state];
      return '<li class="' + (SB.Game.alive(b) ? '' : 'gone') + '"><span class="dot" style="background:' + b.color + '"></span>' +
        '<span class="nm">' + b.name + '</span><span class="st">' + st + '</span>' +
        '<span class="bar"><span style="width:' + Math.round(f * 100) + '%;background:' + hungerColor(f) + '"></span></span></li>';
    }).join('');

    const s = world.stats, secs = Math.floor(world.tick * C.TICK_MS / 1000);
    $('stats').textContent =
      'Time ' + Math.floor(secs / 60) + ':' + String(secs % 60).padStart(2, '0') +
      '   Coins ' + s.coins + '   Bags bought ' + s.bags + '   Sweets eaten ' + s.eaten +
      '   Kicks ' + s.kicks + '   Deaths ' + s.deaths;
    $('log').innerHTML = world.log.map((l, i) =>
      '<li style="opacity:' + (1 - i * 0.14) + '"><span class="dot" style="background:' + l.color + '"></span>' + l.text + '</li>').join('');

    $('btn-sound').firstChild.textContent = SB.Sound.muted ? 'Sound: off ' : 'Sound: on ';
    $('btn-pause').firstChild.textContent = view.paused ? 'Resume ' : 'Pause ';
    $('btn-fast').firstChild.textContent = view.fast ? 'Speed: 2x ' : 'Speed: 1x ';
  }

  SB.Renderer = Renderer;
  SB.updateHud = updateHud;
})(window.SB = window.SB || {});
