/*
 * WGT sprite file (.SPR) reader - a JavaScript port of wloadsprites() from the
 * WordUp Graphics Toolkit (WGT 4.5 / WGT95, LOADSPR.C).
 *
 * File layout (all integers little-endian):
 *   u16        version            (<=3: first sprite is #1, 4: first sprite is #0)
 *   char[13]   " Sprite File "    magic
 *   u8[768]    palette            256 x (r,g,b), 6-bit VGA values 0..63
 *   i16        maxsprite          highest sprite number stored
 *   then for each sprite number from first..maxsprite:
 *     u16      made               1 = sprite present, 0 = empty slot
 *     if made: u16 width, u16 height, u8[width*height] pixels (palette indexes,
 *              row by row). Index 0 is transparent when drawn with wputblock(..., 1).
 */
(function (global) {
  'use strict';

  function base64ToBytes(b64) {
    const bin = atob(b64);
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
    return bytes;
  }

  function parseSpr(bytes) {
    if (!(bytes instanceof Uint8Array)) bytes = new Uint8Array(bytes);
    const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    let o = 0;
    const version = dv.getUint16(o, true); o += 2;
    const magic = String.fromCharCode.apply(null, bytes.subarray(o, o + 13)); o += 13;
    if (magic.toLowerCase() !== ' sprite file ') throw new Error('Not a WGT sprite file (magic was "' + magic + '")');

    const palette = [];
    for (let i = 0; i < 256; i++) {
      // Expand 6-bit VGA DAC values to 8-bit
      const c = [bytes[o], bytes[o + 1], bytes[o + 2]].map(v => (v << 2) | (v >> 4));
      palette.push(c);
      o += 3;
    }

    const maxSprite = dv.getInt16(o, true); o += 2;
    const first = version <= 3 ? 1 : 0;
    const sprites = [];
    for (let i = 0; i < first; i++) sprites.push(null);
    for (let i = first; i <= maxSprite; i++) {
      const made = dv.getUint16(o, true); o += 2;
      if (made !== 1) { sprites.push(null); continue; }
      const w = dv.getUint16(o, true), h = dv.getUint16(o + 2, true); o += 4;
      sprites.push({ w, h, pixels: bytes.slice(o, o + w * h) });
      o += w * h;
    }
    return { version, palette, sprites };
  }

  // Bounding box of the non-transparent pixels (many items sit in the top-left
  // corner of a 64x64 block).
  function bbox(spr) {
    let x0 = spr.w, y0 = spr.h, x1 = -1, y1 = -1;
    for (let y = 0; y < spr.h; y++)
      for (let x = 0; x < spr.w; x++)
        if (spr.pixels[y * spr.w + x]) {
          if (x < x0) x0 = x; if (x > x1) x1 = x;
          if (y < y0) y0 = y; if (y > y1) y1 = y;
        }
    if (x1 < 0) return { x: 0, y: 0, w: 0, h: 0 };
    return { x: x0, y: y0, w: x1 - x0 + 1, h: y1 - y0 + 1 };
  }

  // Render one sprite to a canvas. transparent0: palette index 0 becomes see-through.
  function toCanvas(spr, palette, transparent0 = true) {
    const c = document.createElement('canvas');
    c.width = spr.w; c.height = spr.h;
    const ctx = c.getContext('2d');
    const img = ctx.createImageData(spr.w, spr.h);
    for (let i = 0; i < spr.pixels.length; i++) {
      const idx = spr.pixels[i];
      const rgb = palette[idx];
      img.data[i * 4] = rgb[0];
      img.data[i * 4 + 1] = rgb[1];
      img.data[i * 4 + 2] = rgb[2];
      img.data[i * 4 + 3] = (transparent0 && idx === 0) ? 0 : 255;
    }
    ctx.putImageData(img, 0, 0);
    return c;
  }

  // Parse + pre-render everything the game needs.
  function loadSpriteSet(bytes) {
    const file = parseSpr(bytes);
    const sprites = file.sprites.map(s => s && {
      w: s.w, h: s.h,
      bbox: bbox(s),
      canvas: toCanvas(s, file.palette),
      raw: s,
    });
    return { version: file.version, palette: file.palette, sprites };
  }

  global.WGT = { base64ToBytes, parseSpr, bbox, toCanvas, loadSpriteSet };
})(window);
