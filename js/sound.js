/* Sneaky Beaks - tiny WebAudio sound effects (no audio files needed). */
(function (SB) {
  'use strict';

  let ac = null, muted = false;

  function ensure() {
    if (!ac) { try { ac = new (window.AudioContext || window.webkitAudioContext)(); } catch (e) { ac = null; } }
    if (ac && ac.state === 'suspended') ac.resume();
    return ac;
  }

  function tone(freq, dur, type = 'square', vol = 0.05, slide = 0, delay = 0) {
    if (muted || !ensure()) return;
    const t0 = ac.currentTime + delay;
    const o = ac.createOscillator(), g = ac.createGain();
    o.type = type;
    o.frequency.setValueAtTime(freq, t0);
    if (slide) o.frequency.exponentialRampToValueAtTime(Math.max(20, freq + slide), t0 + dur);
    g.gain.setValueAtTime(vol, t0);
    g.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
    o.connect(g).connect(ac.destination);
    o.start(t0); o.stop(t0 + dur + 0.02);
  }

  SB.Sound = {
    unlock: ensure,
    toggle() { muted = !muted; return muted; },
    get muted() { return muted; },
    set muted(v) { muted = !!v; },
    coin()  { tone(988, 0.08, 'square', 0.04); tone(1319, 0.25, 'square', 0.04, 0, 0.08); },
    land()  { tone(1800, 0.05, 'triangle', 0.03); },
    peck()  { tone(600, 0.04, 'square', 0.03); },
    kick()  { tone(180, 0.12, 'sawtooth', 0.06, -120); },
    spill() { tone(1400, 0.05, 'triangle', 0.03); tone(1100, 0.05, 'triangle', 0.03, 0, 0.05); },
    buy()   { tone(523, 0.08, 'square', 0.04); tone(659, 0.08, 'square', 0.04, 0, 0.08); tone(784, 0.15, 'square', 0.04, 0, 0.16); },
    eat()   { tone(300, 0.06, 'triangle', 0.05, 200); },
    die()   { tone(400, 0.6, 'sawtooth', 0.04, -330); },
    hatch() { tone(700, 0.06, 'square', 0.03); tone(900, 0.06, 'square', 0.03, 0, 0.07); },
  };
})(window.SB = window.SB || {});
