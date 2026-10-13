/*
 * Sneaky Beaks - the simulation (port of BEK_SCR8.C). No drawing or DOM in here.
 *
 * Like the original, every beak (duck) has a STATE and a MOVE state:
 *   noskittle  hungry: races for coins and sweets, chases and kicks carriers
 *   skittle    just ate a sweet: happy for a while (flaps, looks around)
 *   money      carrying a coin to the shop
 *   bag        carrying a bag of sweets: keeps away from hungry ducks, eats
 *   pecking / kicking / kicked / dying   one-shot animations
 *   dead
 * Each tick: items move, every duck runs its state handler (AI + movement),
 * then its animation is advanced, then overlapping ducks are nudged apart.
 */
(function (SB) {
  'use strict';

  const C = SB.config, ANIM = SB.ANIM, Sound = SB.Sound;
  const { MIN_X, MAX_X, MIN_Y, MAX_Y, HUNGER_MAX } = C;

  const S = {
    NOSKITTLE: 'noskittle', SKITTLE: 'skittle', MONEY: 'money', BAG: 'bag',
    PECKING: 'pecking', KICKING: 'kicking', KICKED: 'kicked', DYING: 'dying', DEAD: 'dead',
  };
  const M = {
    NONE: 'none', RANDOM: 'random', LOOKING: 'looking', FLAPPING: 'flapping', EVADE: 'evade',
    TRACK_ITEM: 'track_item', TRACK_B: 'track_b', TRACK_SHOP: 'track_shop', GOBBLE: 'gobble',
  };
  const ONE_SHOT = new Set([S.PECKING, S.KICKING, S.KICKED, S.DYING, S.DEAD]);

  // direction bit masks (same as the original:  9 1 3 / 8 0 2 / 12 4 6)
  const N = 1, E = 2, SO = 4, W = 8;
  const DIRS = [N, N | E, E, SO | E, SO, SO | W, W, N | W];
  const dirVec = d => [(d & E ? 1 : 0) - (d & W ? 1 : 0), (d & SO ? 1 : 0) - (d & N ? 1 : 0)];

  // ---------------------------------------------------------------- helpers
  const rand = n => Math.floor(Math.random() * n);
  const chance = p => Math.random() < p;
  const pick = arr => arr[rand(arr.length)];
  const clamp = (v, a, b) => v < a ? a : v > b ? b : v;
  const dist = (a, b) => Math.hypot(a.x - b.x, a.y - b.y);

  // ------------------------------------------------------------------ world
  const world = {
    beaks: [], items: [], floaters: [], log: [],
    tick: 0, stats: null, shop: null, gameOver: false, nameCounter: 0,
  };

  const alive = b => b.state !== S.DYING && b.state !== S.DEAD;
  // ducks worth kicking (beak_collide in BEK_SCR.C): carrying a coin, a bag, or a sweet just eaten
  const isTarget = b => (b.state === S.MONEY || b.state === S.BAG || b.state === S.SKITTLE) && b.immune === 0;
  // ducks a bag-carrier is afraid of
  const isThreat = b => b.state === S.NOSKITTLE || b.state === S.KICKING;

  function nearest(from, list, filter) {
    let best = null, bestD = Infinity;
    for (const o of list) {
      if (o === from || !filter(o)) continue;
      const d = dist(from, o);
      if (d < bestD) { bestD = d; best = o; }
    }
    return best;
  }

  function say(text, color) {
    world.log.unshift({ text, color });
    if (world.log.length > 6) world.log.pop();
  }
  function floatText(x, y, text, color) {
    world.floaters.push({ x, y, text, color, t: 0 });
  }

  // ------------------------------------------------------------- the shop
  // The shop sign is solid: ducks walk round it and items never land on it,
  // so nothing can be hidden or trapped behind it.
  function initShop(gfx) {
    const bb = gfx.sprites[87].bbox;
    const x = C.SHOP_X, y = C.SHOP_Y;
    const box = { x0: x + bb.x, x1: x + bb.x + bb.w, y0: y + bb.y, y1: y + bb.y + bb.h + 1 };
    world.shop = {
      x, y, frame: 87, box,
      doorX: Math.round((box.x0 + box.x1) / 2), doorY: box.y1 + 2,
    };
  }

  const FEET = 6;   // half-width of a duck's feet
  function inShop(x, y, margin = FEET) {
    const r = world.shop.box;
    return x > r.x0 - margin && x < r.x1 + margin && y > r.y0 && y < r.y1;
  }

  // Move a point (an item or a new duck) out of the shop to its nearest side.
  function outsideShop(p, margin = FEET + 1) {
    if (!inShop(p.x, p.y, margin)) return p;
    const r = world.shop.box;
    const options = [
      { x: r.x0 - margin - 1, y: p.y }, { x: r.x1 + margin + 1, y: p.y },
      { x: p.x, y: r.y0 },              { x: p.x, y: r.y1 },
    ].filter(o => o.x >= MIN_X && o.x <= MAX_X && o.y >= MIN_Y && o.y <= MAX_Y);
    options.sort((a, b) => dist(a, p) - dist(b, p));
    return options[0] || p;
  }

  function segmentHitsShop(ax, ay, bx, by) {
    const steps = Math.ceil(Math.hypot(bx - ax, by - ay) / 2);
    for (let i = 1; i <= steps; i++) {
      const t = i / steps;
      if (inShop(ax + (bx - ax) * t, ay + (by - ay) * t)) return true;
    }
    return false;
  }

  // If the straight line to the goal crosses the shop, find the shortest way
  // round via the shop's corners (Dijkstra over start, 4 corners, goal) and
  // return the first point on that path to walk to.
  function waypoint(b, gx, gy) {
    if (!segmentHitsShop(b.x, b.y, gx, gy)) return [gx, gy];
    const r = world.shop.box, m = FEET + 2;
    const nodes = [[b.x, b.y],
      ...[[r.x0 - m, r.y0], [r.x1 + m, r.y0], [r.x0 - m, r.y1], [r.x1 + m, r.y1]]
        .map(([x, y]) => [clamp(x, MIN_X, MAX_X), clamp(y, MIN_Y, MAX_Y)]),
      [gx, gy]];
    const goal = nodes.length - 1;
    const cost = nodes.map(() => Infinity), prev = nodes.map(() => -1), done = nodes.map(() => false);
    cost[0] = 0;
    for (;;) {
      let u = -1;
      for (let i = 0; i < nodes.length; i++) if (!done[i] && (u < 0 || cost[i] < cost[u])) u = i;
      if (u < 0 || cost[u] === Infinity || u === goal) break;
      done[u] = true;
      for (let v = 1; v < nodes.length; v++) {
        if (done[v] || segmentHitsShop(nodes[u][0], nodes[u][1], nodes[v][0], nodes[v][1])) continue;
        const c = cost[u] + Math.hypot(nodes[v][0] - nodes[u][0], nodes[v][1] - nodes[u][1]);
        if (c < cost[v]) { cost[v] = c; prev[v] = u; }
      }
    }
    if (prev[goal] < 0) return [gx, gy];
    // walk back from the goal to the first node after the start that isn't where we already are
    const path = [];
    for (let v = goal; v > 0; v = prev[v]) path.unshift(nodes[v]);
    return path.find(([x, y]) => x !== b.x || y !== b.y) || [gx, gy];
  }

  // ------------------------------------------------------------- movement
  function setPos(b, x, y) {
    const mx = x - b.x, my = y - b.y;
    b.x = x; b.y = y;
    b.dir = (my < 0 ? N : 0) | (my > 0 ? SO : 0) | (mx > 0 ? E : 0) | (mx < 0 ? W : 0);
    if (mx) b.facingE = mx > 0;
  }

  // Try to move by (mx, my); if that is blocked, slide along one axis.
  function tryMove(b, mx, my) {
    for (const [dx, dy] of [[mx, my], [mx, 0], [0, my]]) {
      const x = clamp(b.x + dx, MIN_X, MAX_X), y = clamp(b.y + dy, MIN_Y, MAX_Y);
      if (x === b.x && y === b.y) continue;
      if (inShop(x, y) && !inShop(b.x, b.y)) continue;
      setPos(b, x, y);
      return true;
    }
    b.dir = 0;
    return false;
  }

  // Walk towards (tx, ty), going round the shop if needed.
  function walkTo(b, tx, ty, speed) {
    const [gx, gy] = waypoint(b, tx, ty);
    const dx = gx - b.x, dy = gy - b.y;
    if (!dx && !dy) { b.dir = 0; return false; }
    return tryMove(b, Math.sign(dx) * Math.min(speed, Math.abs(dx)), Math.sign(dy) * Math.min(speed, Math.abs(dy)));
  }

  // random_1: walk in one direction for a while; pick another if a wall is hit.
  function wander(b, speed = 1) {
    if (!b.wanderDir) return false;               // 0 = standing still for this move
    const [vx, vy] = dirVec(b.wanderDir);
    if (tryMove(b, vx * speed, vy * speed)) return true;
    b.wanderDir = pick(DIRS);
    return false;
  }

  // evade: of the 8 directions, take the one that leaves the duck furthest from
  // the threats, avoiding walls and corners and the shop.
  function evade(b, speed = 2) {
    const threats = C.EVADE_RADIUS * 2;
    const near = world.beaks.filter(o => o !== b && alive(o) && isThreat(o) && dist(b, o) < threats);
    if (!near.length) return wander(b, 1);
    let best = null, bestScore = -Infinity;
    for (const d of DIRS) {
      const [vx, vy] = dirVec(d);
      const x = clamp(b.x + vx * speed, MIN_X, MAX_X), y = clamp(b.y + vy * speed, MIN_Y, MAX_Y);
      if ((x === b.x && y === b.y) || inShop(x, y)) continue;
      let score = 0;
      for (const o of near) score -= 1 / Math.max(4, Math.hypot(x - o.x, y - o.y));
      const edge = Math.min(x - MIN_X, MAX_X - x, (y - MIN_Y) * 1.5, (MAX_Y - y) * 1.5);
      if (edge < 24) score -= (24 - edge) * 0.003;             // don't get cornered
      if (d === b.evadeDir) score += 0.002;                    // keep heading: less jitter
      if (score > bestScore) { bestScore = score; best = d; }
    }
    if (!best) return false;
    b.evadeDir = best;
    const [vx, vy] = dirVec(best);
    return tryMove(b, vx * speed, vy * speed);
  }

  // flapping: a happy duck flaps and takes off up the screen (BEK_SCR.C: y -= 2)
  function flapUp(b) {
    return b.state === S.SKITTLE ? tryMove(b, 0, -2) : false;
  }

  // ------------------------------------------------------------- the beaks
  function makeBeak() {
    const id = world.nameCounter++;
    const p = outsideShop({ x: MIN_X + rand(MAX_X - MIN_X), y: MIN_Y + rand(MAX_Y - MIN_Y) });
    const b = {
      id,
      name: SB.NAMES[id % SB.NAMES.length] + (id >= SB.NAMES.length ? ' ' + (Math.floor(id / SB.NAMES.length) + 1) : ''),
      color: SB.COLORS[id % SB.COLORS.length],
      x: p.x, y: p.y, dir: 0, facingE: chance(0.5),
      state: S.NOSKITTLE, move: M.NONE, moveCount: 0, moveMax: 0, target: null,
      wanderDir: 0, evadeDir: 0, stuck: 0,
      hunger: HUNGER_MAX + rand(80),   // count_beak
      sugar: 0,                        // count_skittle
      bag: 0,                          // bag_count
      hasCoin: false, peckItem: null,
      kickCooldown: 0, immune: 0, kvx: 0, kvy: 0,
      moved: false, diedAt: 0,
      anim: null, frame: 0,
    };
    loopAnim(b, ANIM.walk.s);
    setMove(b, M.FLAPPING, 15);
    return b;
  }

  function setState(b, state, move = M.NONE, moveMax = 3 + rand(5), target = null) {
    b.state = state;
    setMove(b, move, moveMax, target);
  }
  function setMove(b, move, moveMax, target = null) {
    b.move = move; b.moveCount = 0; b.moveMax = moveMax; b.target = target; b.stuck = 0;
    if (move === M.RANDOM) b.wanderDir = chance(0.15) ? 0 : pick(DIRS);
  }

  // ------------------------------------------------------------- animation
  function loopAnim(b, seq, hold = 1) {
    if (b.anim && b.anim.seq === seq) return;
    b.anim = { seq, hold, loop: true, i: 0, t: 0, done: false };
  }
  // one-shot animation; the state waits for anim.done
  function playOnce(b, seq, hold = 1) {
    b.anim = { seq, hold, loop: false, i: 0, t: 0, done: false, fresh: true };
    b.frame = seq[0];
  }
  function stepAnim(b) {
    const a = b.anim;
    if (a.fresh) a.fresh = false;                       // show frame 0 for its full time
    else if (!a.done && ++a.t >= a.hold) {
      a.t = 0;
      if (++a.i >= a.seq.length) {
        if (a.loop) a.i = 0; else { a.i = a.seq.length - 1; a.done = true; }
      }
    }
    b.frame = a.seq[a.i];
  }

  // animation_control()
  function animate(b) {
    if (ONE_SHOT.has(b.state) || b.move === M.GOBBLE) return stepAnim(b);
    const set = b.hasCoin ? ANIM.walkM : b.bag > 0 ? ANIM.walkB : ANIM.walk;
    if (!b.moved && set === ANIM.walk && b.move === M.LOOKING)  { loopAnim(b, ANIM.looking); return stepAnim(b); }
    if (set === ANIM.walk && b.move === M.FLAPPING)             { loopAnim(b, ANIM.flapping, 2); return stepAnim(b); }

    let seq;
    if (b.dir & E) seq = set.e;
    else if (b.dir & W) seq = set.w;
    else if (b.dir & N) seq = set.n;
    else if (b.dir & SO) seq = set.s;
    else seq = [set.e, set.w, set.n].find(s => s === b.anim.seq) || set.s;   // stand facing the same way
    loopAnim(b, seq);
    if (b.moved) stepAnim(b);
    else { b.anim.i = 0; b.anim.t = 0; b.frame = seq[0]; }
  }

  // --------------------------------------------------------------- the items
  // Coins and skittles (sweets). z = height above the ground.
  function dropCoin(x, y) {
    if (world.items.filter(i => i.kind === 'coin').length >= C.MAX_COINS) {
      say('That\'s plenty of coins on the ground!', '#aaaaaa');
      return;
    }
    const p = outsideShop({
      x: Math.round(clamp(x ?? 16 + rand(C.SCREEN_W - 32), MIN_X, MAX_X)),
      y: Math.round(clamp(y ?? MIN_Y + rand(MAX_Y - MIN_Y), MIN_Y, MAX_Y)),
    });
    world.items.push({ kind: 'coin', x: p.x, y: p.y, z: p.y + 16, vx: 0, vy: 0, vz: 0, state: 'fall', fi: 0, frame: ANIM.coin });
    world.stats.coins++;
    Sound.coin();
  }

  // something drops out of a dying duck: a short hop
  function fling(kind, from, toE) {
    const vx = (toE ? 1 : -1) * (1 + rand(3)) + (chance(0.3) ? pick([-1, 1]) : 0);
    throwItem(kind, from, vx, rand(3) - 1, 12, 3 + rand(3));
  }

  // something is kicked out of a duck: BEK_SCR.C throws it at -10..+10 px a tick
  // for a few ticks (coins 4-6, sweets 2-4), so it can land a long way off
  function scatter(kind, from) {
    const ticks = kind === 'coin' ? 4 + rand(3) : 2 + rand(3);
    const z0 = 8;
    throwItem(kind, from, rand(21) - 10, rand(5) - 2, z0, (C.GRAVITY * ticks) / 2 - z0 / ticks);
  }

  function throwItem(kind, from, vx, vy, z, vz) {
    world.items.push({
      kind, x: from.x, y: from.y, z, vx, vy, vz,
      state: 'fly', fi: 0, frame: kind === 'coin' ? ANIM.coin : pick(ANIM.sweets),
    });
  }

  function land(it) {
    const p = outsideShop({ x: clamp(it.x, MIN_X, MAX_X), y: it.y });
    it.x = p.x; it.y = p.y; it.z = 0; it.state = 'ground';
    if (it.kind === 'coin') it.frame = ANIM.coin;
  }

  function updateItems() {
    for (const it of world.items) {
      if (it.state === 'ground') continue;
      if (it.kind === 'coin') { it.fi = (it.fi + 1) % ANIM.coinFlip.length; it.frame = ANIM.coinFlip[it.fi]; }
      if (it.state === 'fall') {                                // drop_money()
        it.z -= C.COIN_FALL;
        if (it.z <= 0) { land(it); Sound.land(); }
      } else {                                                  // bag_kicked()
        it.x = clamp(it.x + it.vx, MIN_X, MAX_X);
        it.y = clamp(it.y + it.vy, MIN_Y, MAX_Y);
        it.z += it.vz; it.vz -= C.GRAVITY;
        if (it.z <= 0) land(it);
      }
    }
  }

  // ------------------------------------------------------------ interactions
  // skittle_collide(): peck up a coin or sweet under the duck's feet
  function tryPickup(b) {
    const items = world.items;
    for (let i = 0; i < items.length; i++) {
      const it = items[i];
      if (it.z > 6 || Math.abs(it.x - b.x) >= 8 || Math.abs(it.y - b.y) >= 6) continue;
      items.splice(i, 1);
      b.peckItem = it;
      b.facingE = it.x >= b.x;
      setState(b, S.PECKING);
      playOnce(b, it.kind === 'coin'
        ? (b.facingE ? ANIM.peckCoinE : ANIM.peckCoinW)
        : (b.facingE ? ANIM.peckE : ANIM.peckW));
      Sound.peck();
      return true;
    }
    return false;
  }

  // beak_collide(): kick a duck that has a coin, a bag of sweets, or a sweet in its mouth
  function tryKick(b) {
    if (b.kickCooldown > 0) return false;
    const v = world.beaks.find(o => o !== b && isTarget(o) && Math.abs(o.x - b.x) < 15 && Math.abs(o.y - b.y) < 8);
    if (!v) return false;
    const toE = v.x >= b.x;
    b.facingE = toE;
    setState(b, S.KICKING);
    playOnce(b, toE ? ANIM.kickE : ANIM.kickW);
    kicked(v, b, toE);
    return true;
  }

  function kicked(v, kicker, toE) {
    world.stats.kicks++;
    Sound.kick();
    let what = '';
    if (v.state === S.MONEY && chance(0.5)) {                   // kicked_m: 50% the money is lost
      v.hasCoin = false;
      scatter('coin', v);
      what = ' - and dropped the coin!';
    } else if (v.state === S.BAG) {                             // kicked_b: 0-4 sweets fly out of the bag
      const n = Math.min(v.bag, rand(5));
      for (let i = 0; i < n; i++) scatter('sweet', v);
      v.bag -= n;
      if (n) { what = ' - ' + n + ' sweet' + (n > 1 ? 's' : '') + ' went flying!'; Sound.spill(); }
    } else if (v.state === S.SKITTLE && chance(0.5)) {          // kicked_s: 50% the sweet is knocked out of its mouth
      v.sugar = 0;
      v.hunger = HUNGER_MAX;                                    // (the original resets its lifetime)
      scatter('sweet', v);
      what = ' - and knocked the sweet out of its mouth!';
      Sound.spill();
    }
    say(kicker.name + ' kicked ' + v.name + what, kicker.color);
    floatText(v.x, v.y - 34, 'KICK!', '#ff5555');
    setState(v, S.KICKED);
    v.kvx = toE ? 4 : -4; v.kvy = rand(3) - 1;
    playOnce(v, toE ? ANIM.tumbleE : ANIM.tumbleW);
  }

  function feed(b) {
    b.hunger = HUNGER_MAX + rand(60);
    world.stats.eaten++;
    Sound.eat();
  }

  function startDying(b) {
    // anything it was holding falls on the floor
    if (b.hasCoin) fling('coin', b, chance(0.5));
    for (let i = 0; i < b.bag; i++) fling('sweet', b, chance(0.5));
    b.hasCoin = false; b.bag = 0;
    setState(b, S.DYING);
    playOnce(b, ANIM.dying);
    b.diedAt = world.tick;
    world.stats.deaths++;
    say(b.name + ' starved to death!', b.color);
    Sound.die();
  }

  // -------------------------------------------------------------- behaviour
  // Pick what a hungry duck does next (weights from BEK_SCR.C):
  //   sweets on the floor  -> track the nearest sweet 2/3, wander 1/3   (beak_noskittle_s)
  //   coins on the floor   -> track the nearest coin 2/3, wander 1/3    (beak_noskittle_m)
  //   floor empty          -> always hunt the nearest duck worth kicking (beak_noskittle_ns_nm)
  function decideHungry(b) {
    const items = world.items;
    const loot = nearest(b, items, i => i.kind === 'sweet') || nearest(b, items, i => i.kind === 'coin');
    if (loot) {
      if (chance(2 / 3)) return setMove(b, M.TRACK_ITEM, 3 + rand(5), loot);
      return setMove(b, M.RANDOM, 8);
    }
    const victim = nearest(b, world.beaks, isTarget);
    if (victim) return setMove(b, M.TRACK_B, 3 + rand(5), victim);
    setMove(b, M.RANDOM, 8);                                     // track_b falls back to random_1
  }

  function targetGone(b) {
    if (b.move === M.TRACK_ITEM) return !world.items.includes(b.target);
    if (b.move === M.TRACK_B) return !isTarget(b.target);
    return false;
  }

  // Carry out the current move state. Returns true if the duck moved.
  function doMove(b) {
    let moved = false, goal = null, speed = 2;
    switch (b.move) {
      case M.RANDOM: return wander(b, 1);
      case M.EVADE: return evade(b);
      case M.FLAPPING: return flapUp(b);
      case M.TRACK_ITEM: goal = b.target; speed = 2 + (chance(0.5) ? 1 : 0); break;
      case M.TRACK_B: goal = b.target; speed = 2 + (chance(0.45) ? 1 : 0); break;
      case M.TRACK_SHOP: goal = { x: world.shop.doorX, y: world.shop.doorY }; break;
      default: return false;
    }
    moved = walkTo(b, goal.x, goal.y, speed);
    // safety net: if a duck can't make progress, let it wander off for a moment
    if (!moved && dist(b, goal) > 2 && ++b.stuck > 12) setMove(b, M.RANDOM, 12 + rand(10));
    return moved;
  }

  // state_control(): one handler per state
  const handlers = {
    [S.NOSKITTLE](b) {
      if (tryPickup(b) || tryKick(b)) return;
      if (b.moveCount >= b.moveMax || targetGone(b)) decideHungry(b);
      b.moved = doMove(b);
    },

    [S.SKITTLE](b) {                                             // just ate a sweet: happy
      b.hunger = HUNGER_MAX;
      if (--b.sugar <= 0) { b.hunger = HUNGER_MAX + rand(60); return setState(b, S.NOSKITTLE, M.RANDOM, 3); }
      // beak_skittle: mostly looks around, sometimes wanders, stands, or flaps and takes off
      if (b.moveCount >= b.moveMax) setMove(b, pick([M.RANDOM, M.NONE, M.LOOKING, M.LOOKING, M.FLAPPING]), 3 + rand(25));
      b.moved = doMove(b);
    },

    [S.MONEY](b) {                                               // take the coin to the shop
      const shop = world.shop;
      if (Math.abs(b.x - shop.doorX) < 6 && Math.abs(b.y - shop.doorY) < 5) {
        b.hasCoin = false;
        b.bag = C.BAG_MAX;
        world.stats.bags++;
        setState(b, S.BAG, M.RANDOM, 6);
        say(b.name + ' bought a bag of ' + C.BAG_MAX + ' sweets', b.color);
        floatText(shop.doorX, shop.doorY - 40, 'KA-CHING!', '#ffe14d');
        Sound.buy();
        return;
      }
      // beak_money: heads for the shop, but 1 time in 4 dawdles for a moment
      if (b.moveCount >= b.moveMax) setMove(b, chance(0.25) ? M.RANDOM : M.TRACK_SHOP, 3 + rand(5));
      b.moved = doMove(b);
    },

    [S.BAG](b) {                                                 // eat sweets, keep away from the others
      if (b.move === M.GOBBLE) {
        if (!b.anim.done) return;
        b.bag--;
        feed(b);
        if (b.bag > 0) return setState(b, S.BAG, M.EVADE, 6);
        say(b.name + ' finished the bag', b.color);
        b.sugar = C.SUGAR_TIME;
        return setState(b, S.SKITTLE, M.FLAPPING, 10);
      }
      if (b.bag <= 0) return setState(b, S.NOSKITTLE, M.RANDOM, 3);

      const threat = nearest(b, world.beaks, o => alive(o) && isThreat(o));
      const safe = !threat || dist(b, threat) > C.EVADE_RADIUS;
      const peckish = b.hunger < HUNGER_MAX * 0.45 || chance(1 / 250);
      if ((safe && peckish) || b.hunger < HUNGER_MAX * 0.15) {   // starving: eat even if chased
        setMove(b, M.GOBBLE, Infinity);
        playOnce(b, pick(ANIM.eating), 2);
        b.moved = false;
        return;
      }
      if (!safe && b.move !== M.EVADE) setMove(b, M.EVADE, 4 + rand(8));
      else if (b.moveCount >= b.moveMax) setMove(b, safe ? pick([M.RANDOM, M.RANDOM, M.NONE, M.EVADE]) : M.EVADE, 4 + rand(12));
      b.moved = doMove(b);
    },

    [S.PECKING](b) {
      if (!b.anim.done) return;
      const it = b.peckItem; b.peckItem = null;
      if (it.kind === 'coin') {
        b.hasCoin = true;
        setState(b, S.MONEY, M.TRACK_SHOP);
        say(b.name + ' grabbed the coin!', b.color);
      } else {
        feed(b);
        b.sugar = C.SUGAR_TIME;
        setState(b, S.SKITTLE, M.FLAPPING, 10);
        floatText(b.x, b.y - 34, 'YUM', '#7dff7d');
      }
    },

    [S.KICKING](b) {
      if (!b.anim.done) return;
      b.kickCooldown = C.KICK_COOLDOWN;
      setState(b, S.NOSKITTLE, M.NONE, 0);                      // decide again next tick
    },

    [S.KICKED](b) {
      tryMove(b, Math.round(b.kvx), b.kvy);                      // knocked back
      b.kvx *= 0.75;
      if (!b.anim.done) return;
      b.immune = C.KICK_IMMUNE;
      if (b.hasCoin) setState(b, S.MONEY, M.TRACK_SHOP);
      else if (b.bag > 0) setState(b, S.BAG, M.EVADE, 8);
      else if (b.sugar > 0) setState(b, S.SKITTLE, M.LOOKING, 10);   // kept its sweet
      else setState(b, S.NOSKITTLE, M.NONE, 0);
    },

    [S.DYING](b) {
      if (b.anim.done) b.state = S.DEAD;
    },

    [S.DEAD]() {},
  };

  // Ducks walking about shouldn't stand inside each other: nudge them apart.
  function separate() {
    const free = world.beaks.filter(b => !ONE_SHOT.has(b.state) && b.move !== M.GOBBLE);
    for (let i = 0; i < free.length; i++)
      for (let j = i + 1; j < free.length; j++) {
        const a = free[i], b = free[j];
        const dx = b.x - a.x;
        if (Math.abs(dx) >= 7 || Math.abs(b.y - a.y) >= 3) continue;
        const s = dx > 0 || (dx === 0 && a.id < b.id) ? 1 : -1;
        nudge(a, -s); nudge(b, s);
      }
  }
  function nudge(b, dx) {
    const x = clamp(b.x + dx, MIN_X, MAX_X);
    if (!inShop(x, b.y)) b.x = x;
  }

  // ------------------------------------------------------------------- tick
  function update() {
    world.tick++;
    updateItems();

    for (const b of world.beaks) {
      if (b.state === S.DEAD) continue;
      b.moveCount++;
      b.moved = false;
      if (b.kickCooldown > 0) b.kickCooldown--;
      if (b.immune > 0) b.immune--;
      if ((b.state === S.NOSKITTLE || b.state === S.MONEY || b.state === S.BAG) && --b.hunger <= 0) startDying(b);
      else handlers[b.state](b);
      animate(b);
    }
    separate();

    // dead ducks leave the stage after a while
    world.beaks = world.beaks.filter(b => b.state !== S.DEAD || world.tick - b.diedAt < 60);
    for (const f of world.floaters) f.t++;
    world.floaters = world.floaters.filter(f => f.t < 18);
    world.shop.frame = ANIM.shop[Math.floor(world.tick / 8) % ANIM.shop.length];

    if (!world.gameOver && world.beaks.length && !world.beaks.some(alive)) {
      world.gameOver = true;
      say('All the ducks have starved. Press R to start again.', '#ff8080');
    }
  }

  // --------------------------------------------------------------- controls
  function newGame() {
    Object.assign(world, { beaks: [], items: [], floaters: [], log: [], tick: 0, gameOver: false, nameCounter: 0 });
    world.stats = { coins: 0, bags: 0, eaten: 0, kicks: 0, deaths: 0 };
    for (let i = 0; i < C.START_BEAKS; i++) world.beaks.push(makeBeak());
    say('Press SPACE (or tap the field) to drop a coin', '#ffe14d');
    dropCoin();                                                  // init_drop_money(): the 1st coin
  }

  function hatch() {
    if (world.beaks.filter(alive).length >= C.MAX_BEAKS) {
      say('The pond is full (max ' + C.MAX_BEAKS + ' ducks)', '#aaaaaa');
      return;
    }
    const b = makeBeak();
    world.beaks.push(b);
    world.gameOver = false;
    say(b.name + ' has arrived', b.color);
    floatText(b.x, b.y - 34, 'HI', '#ffffff');
    Sound.hatch();
  }

  SB.Game = { world, S, alive, initShop, newGame, update, dropCoin, hatch };
})(window.SB = window.SB || {});
