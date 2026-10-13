/* Sneaky Beaks - start-up, input and the main loop. */
(function (SB) {
  'use strict';

  const C = SB.config, Game = SB.Game, Sound = SB.Sound;
  const view = { paused: false, fast: false, showBars: true, classicBg: false };
  let canvas, renderer;

  const actions = {
    coin()    { if (!view.paused) Game.dropCoin(); },
    duck()    { Game.hatch(); },
    pause()   { view.paused = !view.paused; },
    fast()    { view.fast = !view.fast; },
    restart() { Game.newGame(); },
    bars()    { view.showBars = !view.showBars; },
    bg()      { view.classicBg = !view.classicBg; renderer.setBackground(Game.world.shop, view.classicBg); },
    sound()   { Sound.toggle(); },
  };
  const KEYS = { ' ': 'coin', m: 'coin', n: 'duck', p: 'pause', f: 'fast', r: 'restart', h: 'bars', b: 'bg', s: 'sound' };

  function run(name) {
    Sound.unlock();
    actions[name]();
    SB.updateHud(Game.world, view);
  }

  function resize() {
    const availW = document.getElementById('stage').clientWidth;
    const availH = Math.max(200, window.innerHeight - 230);
    let scale = Math.min(availW / C.SCREEN_W, availH / C.SCREEN_H);
    if (scale >= 1) scale = Math.floor(scale);                    // integer scaling keeps pixels crisp
    canvas.style.width = Math.floor(C.SCREEN_W * scale) + 'px';
    canvas.style.height = Math.floor(C.SCREEN_H * scale) + 'px';
  }

  function start() {
    const gfx = WGT.loadSpriteSet(WGT.base64ToBytes(window.BEAK_SPR_BASE64));
    SB.patchSprites(gfx);

    canvas = document.getElementById('screen');
    renderer = SB.Renderer(canvas, gfx);
    Game.initShop(gfx);
    renderer.setBackground(Game.world.shop, view.classicBg);
    Game.newGame();

    window.addEventListener('keydown', e => {
      if (e.ctrlKey || e.metaKey || e.altKey || /INPUT|TEXTAREA|SELECT/.test(e.target.tagName)) return;
      const name = KEYS[e.key.toLowerCase()];
      if (!name) return;
      e.preventDefault();
      run(name);
    });
    for (const name of Object.keys(actions)) {
      const el = document.getElementById('btn-' + name);
      if (el) el.addEventListener('click', () => run(name));
    }
    canvas.addEventListener('pointerdown', e => {               // drop a coin where you click
      Sound.unlock();
      if (view.paused) return;
      const r = canvas.getBoundingClientRect();
      Game.dropCoin((e.clientX - r.left) / r.width * C.SCREEN_W, (e.clientY - r.top) / r.height * C.SCREEN_H);
    });
    window.addEventListener('resize', resize);
    resize();

    // debug: index.html?sim=2000&coins=60 fast-forwards 2000 ticks, dropping a coin every 60 ticks
    const qs = new URLSearchParams(location.search);
    const sim = +qs.get('sim'), every = qs.has('coins') ? +qs.get('coins') : 60;
    if (sim > 0) {
      const wasMuted = Sound.muted;
      Sound.muted = true;
      for (let i = 0; i < sim; i++) { if (every > 0 && i % every === every - 1) Game.dropCoin(); Game.update(); }
      Sound.muted = wasMuted;
    }

    // fixed-rate game ticks (like the original delay(70)), drawn with requestAnimationFrame
    let last = performance.now(), acc = 0, hudT = 0;
    function frame(now) {
      const dt = Math.min(250, now - last);
      last = now;
      if (!view.paused) {
        acc += dt * (view.fast ? 2 : 1);
        while (acc >= C.TICK_MS) { Game.update(); acc -= C.TICK_MS; }
      }
      renderer.draw(Game.world, view);
      if ((hudT += dt) > 200) { hudT = 0; SB.updateHud(Game.world, view); }
      requestAnimationFrame(frame);
    }
    SB.updateHud(Game.world, view);
    requestAnimationFrame(frame);
  }

  window.addEventListener('DOMContentLoaded', start);
})(window.SB = window.SB || {});
