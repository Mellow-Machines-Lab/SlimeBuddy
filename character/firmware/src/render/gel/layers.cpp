#include "render/gel/layers.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "render/font.h"
#include "render/gel/engine.h"
#include "render/gel/lane.h"
#include "render/gel/raster.h"
#include "render/maths.h"
#include "render/palette.h"
#include "render/tags.h"

namespace render::gel {

namespace {

// The props' colours (render/gel/props.cpp): a card's ink and rim, its
// words, and the helpers' gel for the hands.
constexpr uint16_t kCardInk = rgb565(27, 55, 78), kCardRim = rgb565(0x78, 0xa4, 0xc8);
constexpr uint16_t kWords = rgb565(184, 233, 245), kGel = rgb565(89, 169, 224), kShine = rgb565(184, 233, 245);
constexpr uint16_t kCream = rgb565(246, 240, 226), kInk = rgb565(28, 24, 36);
// The nightcap's flannel, its stripes and the cap's
// cuff, and the pale gold of a wish's sparks.
constexpr uint16_t kNightcloth = rgb565(62, 72, 168), kNightstripe = rgb565(176, 190, 238),
                   kNightcuff = rgb565(112, 124, 210), kMoonlight = rgb565(250, 230, 160);

// The point `i` thirty-seconds of a turn round a circle, as cosine and
// sine (Q12).
Point turn(int i) {
  static constexpr int16_t kCos[] = {4096, 4017, 3784, 3406, 2896, 2276, 1567, 799, 0};  // a quarter's
  const int j = i % 8;
  switch (i / 8 % 4) {
    case 0: return {kCos[j], kCos[8 - j]};
    case 1: return {-kCos[8 - j], kCos[j]};
    case 2: return {-kCos[j], -kCos[8 - j]};
    default: return {kCos[8 - j], -kCos[j]};
  }
}

// The ring through its left side, its front and its right side, as a
// curve (sixteenths), its `n` + 1 points from left to right, raised `up`.
int ring(const rig::Point& l, const rig::Point& f, const rig::Point& r, int up, Point* out, int n = 8) {
  const int cx = 2 * f.x - (l.x + r.x) / 2, cy = 2 * f.y - (l.y + r.y) / 2;  // through f at the middle
  for (int i = 0; i <= n; ++i) {
    const int64_t u = i, v = n - i, d = int64_t(n) * n;
    out[i] = {int((v * v * l.x + 2 * u * v * cx + u * u * r.x) / d),
              int((v * v * l.y + 2 * u * v * cy + u * u * r.y) / d) - up};
  }
  return n + 1;
}

// A brim's curve and the head's width across it (sixteenths).
struct Head {
  Point band[9];
  int width, cx, top;
};
Head head(const rig::Anchors& at, int up) {
  Head h;
  ring(at.brimL, at.brimF, at.brimR, up, h.band);
  h.width = std::max(16, at.brimR.x - at.brimL.x);
  h.cx = (at.brimL.x + at.brimR.x) / 2;
  h.top = at.crown.y;
  return h;
}

// A triangle wave, 0 to `amp` and back, once every `period` ms.
int wave(int ms, int period, int amp) {
  const int p = ms % period;
  return (p < period / 2 ? p : period - p) * 2 * amp / period;
}
// The same, in sixteenths of a pixel.
int wave16(int ms, int period, int amp) { return wave(ms, period, amp * 16); }

// A long sleeping cap in striped flannel: up from a soft cuff, over the
// crown and flopping down its left side, its tassel hanging by its cheek
// and swaying out and back every 2 s: `ms` into the sway.
void nightcap(Raster& r, const Head& h, int ms) {
  const int w = h.width, sway = wave(ms, 2000, w / 24);
  // Thousandths of the head's width from the crown, and how far down the
  // flop each is (it sways more the lower it hangs).
  auto at = [&](int x, int y, int hang = 0) {
    return Point{h.cx + w * x / 1000 + sway * hang / 4, h.top + w * y / 1000};
  };
  Point v[28];
  int n = 0;
  for (const Point& p : h.band) v[n++] = p;
  v[n++] = at(380, -40);  // up its right side
  v[n++] = at(230, -210);
  v[n++] = at(80, -320);
  v[n++] = at(-120, -370);  // over the top, where it bends
  v[n++] = at(-330, -350, 1);
  v[n++] = at(-520, -250, 2);  // and down the flop's outside
  v[n++] = at(-640, -60, 3);
  v[n++] = at(-690, 150, 4);
  v[n++] = at(-700, 330, 5);
  v[n++] = at(-625, 310, 5);  // back up its inside
  v[n++] = at(-600, 110, 4);
  v[n++] = at(-545, -80, 3);
  v[n++] = at(-450, -190, 2);
  v[n++] = at(-330, -240, 1);
  v[n++] = at(-420, -60);  // and down the cap's left side to the cuff
  r.polygon16(v, n, kNightcloth);
  for (int i = -2; i < 4; ++i) {  // pale stripes across it, cut to the cap
    const int y0 = h.top + w * (60 - i * 130) / 1000, y1 = y0 - w * 55 / 1000;
    const Point stripe[] = {{h.cx - w, y0}, {h.cx + w, y0}, {h.cx + w, y1}, {h.cx - w, y1}};
    r.polygon16(stripe, 4, kNightstripe, v, n);
  }
  r.stroke16(h.band, 9, w / 14, kNightcuff);  // the cuff, rolled
  const Point cord[] = {at(-662, 320, 5), at(-665, 390, 6)};
  r.stroke16(cord, 2, w / 60 + 8, kNightstripe);
  r.disk16(cord[1].x, cord[1].y, w / 22, kNightstripe);  // the tassel
}

// A small cone perched on top: its base the middle of the brim, drawn in
// toward the crown, and a pompom.
void partyHat(Raster& r, const Head& h, uint16_t a, uint16_t b) {
  const int w = h.width;
  const Point tip{h.cx + w / 24, h.top - w * 9 / 20};
  Point v[16], stripe[4], base[9];
  int n = 0;
  for (int i = 0; i < 9; ++i)
    base[i] = {h.cx + (h.band[i].x - h.cx) * 11 / 20, h.band[i].y - (h.band[i].y - h.top) * 2 / 5};
  for (const Point& p : base) v[n++] = p;
  v[n++] = tip;
  r.polygon16(v, n, a);
  for (int i = 1; i <= 2; ++i) {  // two stripes, cut to the cone
    const int y0 = h.top - w * (i * 14) / 100 + w / 20, y1 = y0 - w / 16;
    stripe[0] = {h.cx - w, y0}, stripe[1] = {h.cx + w, y0}, stripe[2] = {h.cx + w, y1}, stripe[3] = {h.cx - w, y1};
    r.polygon16(stripe, 4, b, v, n);
  }
  r.stroke16(base, 9, w / 16, b);
  r.disk16(tip.x, tip.y, w / 16, b);
}

constexpr int kWitchTallMost = 80 * 16;
void witchHat(Raster& r, const Head& h) {
  const int w = h.width;
  const uint16_t felt = rgb565(52, 34, 70);
  Point brim[9];  // wider than the head
  for (int i = 0; i < 9; ++i) brim[i] = {h.cx + (h.band[i].x - h.cx) * 7 / 5, h.band[i].y};
  r.stroke16(brim, 9, w / 10, felt);
  Point v[16];
  int n = 0;
  for (int i = 1; i < 8; ++i) v[n++] = {h.cx + (h.band[i].x - h.cx) * 3 / 4, h.band[i].y};
  // A big head's stops short of the screen's top, so the tip stays on it.
  const int tall = std::min(w * 3 / 5, kWitchTallMost);
  v[n++] = {h.cx + w / 6, h.top - tall * 7 / 12};
  v[n++] = {h.cx + w * 2 / 5, h.top - tall};  // the bent tip
  v[n++] = {h.cx - w / 20, h.top - tall / 2};
  r.polygon16(v, n, felt);
  Point band[7];
  for (int i = 0; i < 7; ++i) band[i] = {v[i].x, v[i].y - w / 14};
  r.stroke16(band, 7, w / 14, rgb565(240, 140, 40));
}

void bobble(Raster& r, const Head& h) {
  const int w = h.width;
  const uint16_t knit = rgb565(200, 52, 60);
  Point v[16];
  int n = 0;
  for (const Point& p : h.band) v[n++] = p;
  // A dome over the crown, right to left.
  v[n++] = {h.cx + w * 2 / 5, h.top - w / 5};
  v[n++] = {h.cx + w / 5, h.top - w * 3 / 10};
  v[n++] = {h.cx, h.top - w * 8 / 25};
  v[n++] = {h.cx - w / 5, h.top - w * 3 / 10};
  v[n++] = {h.cx - w * 2 / 5, h.top - w / 5};
  r.polygon16(v, n, knit);
  r.stroke16(h.band, 9, w / 8, kCream);
  r.disk16(h.cx, h.top - w * 8 / 25, w / 11, kCream);
}

void shades(Raster& r, const rig::Anchors& at) {
  const int d = std::max(16, at.eyeR.x - at.eyeL.x), lw = d * 27 / 100, lh = d / 6;
  for (const rig::Point& e : {at.eyeL, at.eyeR}) {
    const Point lens[] = {
        {e.x - lw, e.y - lh}, {e.x + lw, e.y - lh}, {e.x + lw * 4 / 5, e.y + lh}, {e.x - lw * 4 / 5, e.y + lh}};
    r.polygon16(lens, 4, kInk);
    r.alpha = 150;
    r.disk16(e.x - lw / 2, e.y - lh / 2, d / 40 + 8, kCream);
    r.alpha = 255;
  }
  const Point bridge[] = {{at.eyeL.x + lw, at.eyeL.y - lh / 2}, {at.eyeR.x - lw, at.eyeR.y - lh / 2}};
  r.stroke16(bridge, 2, d / 30 + 16, kInk);
}

// A heart about (x16, y16), sixteenths of a pixel, `s` whole pixels its
// half-width.
void heart(Raster& r, int x16, int y16, int s, uint16_t ink) {
  const int half = s / 2 * 16, lobe = std::max(1, s / 2) * 16 + 8;
  r.disk16(x16 - half + 8, y16 + 8, lobe, ink);
  r.disk16(x16 + half + 8, y16 + 8, lobe, ink);
  const Point v[] = {{x16 - s * 16 + 8, y16 + 24}, {x16 + s * 16 + 8, y16 + 24}, {x16 + 8, y16 + (s + s / 2) * 16 + 8}};
  r.polygon16(v, 3, ink);
}

// The wish's icons in its thought bubble, about (cx, cy), whole pixels. Showing
// how, they move, saying what to do: the hand slides across, as a stroke does;
// the speech dots pulse, one after another; the laptop's screen blinks.
void wishIcon(Raster& r, const WishShow& wish, int cx, int cy, uint16_t ink) {
  constexpr int kPeriodMs = 1400;
  const int phase = int(wish.ms % kPeriodMs);
  switch (wish.kind) {
    case WishShow::kPet: {  // an open hand: a palm and four fingers
      if (wish.how) cx += 8 - wave(phase, kPeriodMs, 16);
      r.round(cx - 9, cy - 1, 18, 13, 5, ink);
      for (int i = 0; i < 4; ++i) r.line({cx - 7 + i * 5, cy + 1}, {cx - 7 + i * 5, cy - 11 + (i == 0 || i == 3 ? 3 : 0)}, 4, ink);
      break;
    }
    case WishShow::kChat:  // a speech dot: three in a row
      for (int i = -1; i <= 1; ++i) {
        const bool lit = wish.how && phase * 3 / kPeriodMs == i + 1;
        r.disk(cx + i * 10, cy, lit ? 6 : 4, ink);
      }
      break;
    case WishShow::kWork:  // a little laptop: its screen and its base
      r.ring(cx - 11, cy - 10, 22, 15, 3, 3, ink);
      r.round(cx - 15, cy + 7, 30, 4, 2, ink);
      if (wish.how && phase < kPeriodMs / 2) r.round(cx - 6, cy - 5, 12, 5, 2, ink);
      break;
    case WishShow::kNone:
      break;
  }
}

// Zs drifting up and right from beside its head, each for 2.4 s, a new
// one every 0.8 s, growing from 7 to 16 px tall by sixteenths of a pixel,
// fading in over its first 200 ms and out to nothing by its last, so none
// pops in or off.
void zs(Raster& r, const rig::Anchors& at, int ms) {
  for (int i = 0; i < 3; ++i) {
    const int age = (ms + i * 800) % 2400;
    r.alpha = std::min(age * 255 / 200, 255 - age * 255 / 2400);
    r.zee(at.brimR.x + 25 * 16 + age * 16 / 40, at.brimR.y - 5 * 16 - age * 16 / 20 - age * 5 * 16 / 2400,
          7 * 16 + age * 9 * 16 / 2400, kCream);
  }
  r.alpha = 255;
}

// What it wears besides where it sits: the hat, the party hat's colours,
// how far into its swing the nightcap is, on a step of kSwingMs (the hat's
// as one number), and what's over its eyes.
struct Look {
  Outfit hat;
  int colors, swing;
  Eyewear eyes;
  int size = 4096;  // the body's (BodySettings)
  uint32_t hatKey() const { return uint32_t(hat) | uint32_t(colors) << 4 | uint32_t(swing) << 8; }
};
// The nightcap's swing for the engine's clock.
int swingFor(const Engine& e) { return int(e.elapsedMs % 2000 / OutfitLayer::kSwingMs * OutfitLayer::kSwingMs); }
// Peekaboo's gesture takes it all off, and it sleeps with its eyes bare;
// the shades come off while it shows a feeling (a reaction, a face the
// Mac lends it, needs you, listening), so its eyes are seen.
Look lookFor(const Engine& e) {
  const uint32_t when = e.scenes.when;
  const SceneState state = e.scenes.route.state;
  if (state == SceneState::kGesture && e.scenes.variant == int(Gesture::kPeekaboo)) return {};
  const Outfit hat = wornHat(when);
  const int colors = hasTag(when, "lunar") ? 1 : hasTag(when, "programmers") && !hasTag(when, "party") ? 2 : 0;
  const bool nightcap = hat == Outfit::kNightcap;
  const bool feeling = e.router.reacting || e.router.expression >= 0 ||
                       state == SceneState::kNeedsYou || state == SceneState::kListening;
  const Eyewear worn = state == SceneState::kAsleep ? Eyewear::kNone : eyewearFor(when);
  const Eyewear eyes = worn == Eyewear::kShades && feeling ? Eyewear::kNone : worn;
  return {hat, hat == Outfit::kPartyHat ? colors : 0, nightcap ? swingFor(e) : 0, eyes};
}

void eyewear(Raster& r, const rig::Anchors& at, Eyewear eyes) {
  if (eyes == Eyewear::kShades) shades(r, at);
}

void hat(Raster& r, const rig::Anchors& at, const Look& look) {
  Head h = head(at, 0);
  // A Chonk is wider, not taller: its hat stands as high as a Buddy's, with
  // the same room over its head, on its wider brim.
  if (look.size > kBuddyBody.size) h.width = h.width * kBuddyBody.size / look.size;
  switch (look.hat) {
    case Outfit::kWitchHat: witchHat(r, h); break;
    case Outfit::kPartyHat: {
      static constexpr uint16_t kColors[][2] = {{rgb565(240, 80, 150), rgb565(250, 210, 70)},  // a party's
                                                {rgb565(210, 40, 40), rgb565(250, 200, 60)},   // Lunar New Year's
                                                {rgb565(30, 40, 36), rgb565(90, 230, 120)}};   // Programmers' Day's
      partyHat(r, h, kColors[look.colors][0], kColors[look.colors][1]);
      break;
    }
    case Outfit::kNightcap: nightcap(r, h, look.swing); break;
    case Outfit::kBobble: bobble(r, h); break;
    default: break;
  }
}

// The anchors `at` moved `dx`, `dy` sixteenths.
rig::Anchors moved(rig::Anchors at, int dx, int dy) {
  for (Point* p : {&at.crown, &at.brimL, &at.brimF, &at.brimR, &at.handF, &at.eyeL, &at.eyeR, &at.mouth}) p->x += dx, p->y += dy;
  for (int i = 0; i < rig::Anchors::kRingPoints; ++i) {
    at.lap[i].x += dx, at.lap[i].y += dy;
    at.hem[i].x += dx, at.hem[i].y += dy;
  }
  return at;
}

// Part of what it wears as last drawn into a recording (OutfitLayer): what
// it looked like, the anchors it was drawn at, the one it rides first, and
// the last frame it was blended into.
struct Kept {
  static constexpr int kAnchors = 6;
  Recording shapes;
  bool valid = false;
  uint32_t look = 0;
  Point at[kAnchors]{};
  int dx = 0, dy = 0;  // the surface's pixels from the recording's
  // The last frame's body, time and anchors, and when its anchors last
  // moved.
  const Engine* engine = nullptr;
  uint32_t ms = 0, movedAt = 0;
  Point last[kAnchors]{};
};
// Room for the hat's and the eyewear's recordings: one block made at start,
// so on the board it's PSRAM's, not its scarce internal memory's (17 KB).
struct Rooms {
  uint8_t hat[OutfitLayer::kHatBytes], worn[OutfitLayer::kWornBytes];
};
Rooms& rooms = *new Rooms;
// The hat, on the brim's front; and what's over its eyes, on the left eye.
Kept keptHat{{rooms.hat, OutfitLayer::kHatBytes}}, keptWorn{{rooms.worn, OutfitLayer::kWornBytes}};

int floor16(int v) { return v >= 0 ? v / 16 : -((15 - v) / 16); }

// `draw` at `at`, which looks like `look` and rides the `n` anchors `now`:
// blended from what `kept` holds while that fits, else drawn into its
// recording first; drawn straight on the surface when it doesn't fit the
// recording's frame or its room.
template <typename Draw>
void wear(Raster& r, Kept& kept, const Engine& e, const rig::Anchors& at, const Point* now, int n, uint32_t look,
          Draw draw) {
  // Moving: a frame of the same body just before, and its anchors moved
  // within kStillMs.
  const uint32_t t = e.elapsedMs;
  for (int i = 0; i < n; ++i)
    if (now[i].x != kept.last[i].x || now[i].y != kept.last[i].y) kept.movedAt = t;
  const bool moving = kept.engine == &e && t != kept.ms && t - kept.ms <= OutfitLayer::kMovingMs &&
                      t - kept.movedAt < OutfitLayer::kStillMs;
  kept.engine = &e, kept.ms = t;
  std::copy(now, now + n, kept.last);
  // How far the body has moved it since it was kept, in sixteenths, by the
  // anchor it rides, and the most any other is off once it's moved that
  // far: how far its shape has changed. Still, it's drawn afresh unless it
  // moved whole pixels and kept its shape.
  int ox = 0, oy = 0, off = INT_MAX;
  if (kept.valid && kept.look == look) {
    ox = now[0].x - kept.at[0].x, oy = now[0].y - kept.at[0].y;
    off = 0;
    for (int i = 0; i < n; ++i)
      off = std::max({off, std::abs(now[i].x - kept.at[i].x - ox), std::abs(now[i].y - kept.at[i].y - oy)});
  }
  const bool whole = off == 0 && ox % 16 == 0 && oy % 16 == 0;
  if (!whole && !(moving && off <= OutfitLayer::kSlack)) {
    // Kept in a frame as wide as the surface and taller, the anchor it rides
    // a whole number of pixels from its middle, as far into its pixel as it
    // is.
    const int fx = r.s.width / 2 - floor16(now[0].x), fy = OutfitLayer::kFrameY - floor16(now[0].y);
    Raster record{r.s};
    record.limit = OutfitLayer::kFrameHeight;
    record.recording = &kept.shapes;
    kept.shapes.clear();
    draw(record, moved(at, fx * 16, fy * 16));
    const Box& b = kept.shapes.box;
    kept.valid = !kept.shapes.full && b.x0 > 0 && b.y0 > 0 && b.x1 < r.s.width && b.y1 < OutfitLayer::kFrameHeight;
    if (!kept.valid) return draw(r, at);  // past the frame's edge, or its room
    kept.look = look, kept.dx = -fx, kept.dy = -fy, ox = oy = 0;
    std::copy(now, now + n, kept.at);
  }
  r.replay16(kept.shapes, kept.dx * 16 + ox, kept.dy * 16 + oy);
}

// The worn and hat layers' contents for a look, and the look a pair of
// them is, its nightcap swinging on the engine's clock.
uint32_t hatOf(const Look& l) { return l.hat == Outfit::kNone ? 0 : uint32_t(l.hat) | uint32_t(l.colors) << 4; }
uint32_t wornOf(const Look& l) { return uint32_t(l.eyes); }
Look lookOf(uint32_t hat, uint32_t worn, const Engine& e) {
  const Outfit kind = Outfit(hat & 15);
  return {kind, int(hat >> 4 & 15), kind == Outfit::kNightcap ? swingFor(e) : 0,
          Eyewear(worn), e.body().size};
}

// What's over its eyes, then the hat, each at its layer's fade.
void outfit(Raster& r, const Engine& e, const rig::Anchors& at, const Look& look, int wornFade, int hatFade) {
  r.fade = wornFade;
  if (look.eyes != Eyewear::kNone) {
    const Point now[] = {at.eyeL, at.eyeR};
    wear(r, keptWorn, e, at, now, 2, uint32_t(look.eyes),
         [&look](Raster& r, const rig::Anchors& at) { eyewear(r, at, look.eyes); });
  }
  r.fade = hatFade;
  if (look.hat != Outfit::kNone) {
    const Point now[] = {at.brimF, at.crown, at.brimL, at.brimR};
    wear(r, keptHat, e, at, now, 4, look.hatKey(), [&look](Raster& r, const rig::Anchors& at) { hat(r, at, look); });
  }
  r.fade = 255;
}

}  // namespace

int sleepyLids(const Engine& e) {
  if (!hasTag(e.scenes.when, "night")) return 0;
  // They go as the face squashes shut, gone by when it's a line, and come
  // back as it wakes.
  const F open = max(0, 1 - e.face.shut.x / SemanticFace::kShutLine);
  return int((open.raw * kNightLids + F::scale / 2) / F::scale);
}

Contents layerContents(const Engine& e) {
  const Look look = lookFor(e);
  const bool zs = hasTag(e.scenes.when, "night") && e.scenes.route.state == SceneState::kAsleep;
  return {wornOf(look), hatOf(look), zs ? kZs : 0u};
}

void paintLayers(Surface& s, const Engine& e, const rig::Anchors& at, const rig::Layers* layers, uint32_t t) {
  if (!s.pixels) return;
  Raster r{s};
  const int ms = int(e.elapsedMs % 3600000u);
  // Each layer's content and fade: the stack's, or with none, whole in
  // what the engine's tags and state call for.
  using rig::Layer;
  const Contents now = layerContents(e);
  auto content = [&](Layer l, uint32_t whole) { return layers ? layers->content(l, t) : whole; };
  auto fade = [&](Layer l) { return layers ? (layers->opacity(l, t) * 255 + 500) / 1000 : 255; };
  outfit(r, e, at, lookOf(content(Layer::kHat, now.hat), content(Layer::kWorn, now.worn), e), fade(Layer::kWorn),
         fade(Layer::kHat));
  r.fade = fade(Layer::kBeside);
  if (content(Layer::kBeside, now.beside) == kZs) zs(r, at, ms);
}

namespace {
constexpr int kWishRadius = 26;
// The wish bubble's middle: up and to the right of the head, held on the panel.
Point wishCentre(const Surface& s, const rig::Anchors& at) {
  return {std::min(s.width - kWishRadius - 8, at.brimR.x / 16 + 30), std::max(kWishRadius + 6, at.crown.y / 16 + 6)};
}
}  // namespace

void paintWish(Surface& s, const WishShow& wish, const rig::Anchors& at) {
  if (wish.kind == WishShow::kNone || !s.pixels) return;
  Raster r{s};
  constexpr int kRadius = kWishRadius;
  const Point centre = wishCentre(s, at);
  const int cx = centre.x, cy = centre.y;
  const int age = int(wish.popAge), pop = int(WishShow::kPopMs);
  // A wish goes by shrinking, as a cloud thins: its disks overlap, and fading
  // them would leave a muddy patch where they do. A wish left goes at once; one
  // met turns white once its heart has popped, or as it flies off.
  const int goes = wish.pop == WishShow::kGone ? 0 : wish.flies ? int(GrowthShow::kHeartWaitMs) : pop / 2;
  const int gone = wish.pop == WishShow::kWaiting || age <= goes ? 0 : ease1000(age - goes, pop - goes);
  const auto size = [gone](int radius) { return radius * (1000 - gone) / 1000; };
  const uint16_t cloud = gone && wish.pop == WishShow::kMet ? rgb565(255, 255, 255) : kCream;
  // The thought's trail down to the head, then the cloud.
  r.disk(cx - kRadius - 6, cy + kRadius + 6, size(4), cloud);
  r.disk(cx - kRadius + 4, cy + kRadius - 3, size(7), cloud);
  r.disk(cx - 10, cy + 8, size(kRadius - 8), cloud);
  r.disk(cx + 10, cy + 8, size(kRadius - 8), cloud);
  r.disk(cx, cy - 4, size(kRadius - 2), cloud);
  if (wish.pop == WishShow::kMet) {
    // The icon pops into a heart, with sparks flying out from it.
    constexpr uint16_t kHeart = rgb565(226, 78, 104);
    if (!wish.flies || age < int(GrowthShow::kHeartWaitMs))
      heart(r, cx * 16, (cy - 10) * 16, size(std::min(18, 4 + age / 20)), kHeart);
    r.alpha = 255 * std::max(0, pop - age) / pop;
    for (int i = 0; i < 6; ++i) {
      const Point out = turn(i * 32 / 6);
      const int d = kRadius + age / 18;
      r.disk(cx + d * out.x / 4096, cy + d * out.y / 4096, 2, kMoonlight);
    }
  } else {
    r.alpha = 255 * (1000 - gone) / 1000;
    wishIcon(r, wish, cx, cy, kInk);
  }
  r.alpha = 255;
}

void paintGrowthBar(Surface& s, const GrowthShow& bar, const rig::Anchors& at, bool hat, int yield) {
  if (!s.pixels || !bar.opacity) return;
  Raster r{s};
  // The bar centred over the body, and how much today straight after it,
  // the number alone in the large font: the bar is today.
  constexpr int kWidth = 200, kHeight = 18, kTop = 18, kSlide = 40, kNumberGap = 10;
  constexpr uint16_t kTrack = rgb565(22, 34, 46), kRim = rgb565(70, 102, 126), kFill = rgb565(89, 169, 224),
                     kFillTop = rgb565(170, 222, 245), kFull = rgb565(kGoldRgb), kFullTop = rgb565(255, 234, 160),
                     kHeart = rgb565(226, 78, 104), kGlow = rgb565(255, 250, 220), kNumber = rgb565(kEyeRgb);
  const int x = (s.width - kWidth) / 2, y = kTop - kSlide * (255 - bar.opacity) / 255;
  // Today's ring, full: gold, and nothing more counts today.
  const bool last = bar.fill >= 1000;
  // Swells as it lands, and settles.
  const int swell = bar.gulp ? 4 * (bar.gulp < 300 ? bar.gulp : (1000 - bar.gulp) * 300 / 700) / 300 : 0;
  const int top = y - swell / 2, height = kHeight + swell, cy = y + kHeight / 2;
  const int tip = x + 2 + (kWidth - 4) * bar.fill / 1000;
  // What else holds the top band (LISTENING) takes it: the bar gives way.
  const int shown = bar.opacity * std::clamp(yield, 0, 255) / 255;
  r.alpha = shown;
  // A hat reaches the bar's row: it goes behind the bar, with no scrap of its
  // tip above it. (With the crown under the bar, so never over the body.)
  if (hat && top > 0 && at.crown.y / 16 > y + kHeight && shown > 2) r.round(s.width / 2 - 110, 0, 220, top, 0, 0);
  r.round(x, top, kWidth, height, height / 2, kTrack);
  if (tip > x + 4) {
    const int w = tip - x - 2;
    r.round(x + 2, top + 2, w, height - 4, (height - 4) / 2, last ? kFull : kFill);
    if (w > 12) r.round(x + 7, top + 3, w - 10, 2, 1, last ? kFullTop : kFillTop);
  }
  r.ring(x, top, kWidth, height, height / 2, 1, kRim);
  // How much today, counting up as it fills.
  char today[8];
  snprintf(today, sizeof today, "%d", (bar.fill + 5) / 10);
  if (shown > 2) text(r, kLarge, x + kWidth + kNumberGap, cy - kLarge.h / 2, today, kNumber);
  if (bar.sparkle) {
    // Full: stars burst off along the bar, and fade, rising no more than
    // 12 px, so the panel's top edge never cuts them.
    const int age = bar.sparkle;
    r.alpha = shown * (1000 - age) / 1000;
    for (int i = 0; i < 7; ++i) {
      const int sx = x + 20 + i * (kWidth - 40) / 6, rise = 4 + age * (4 + (i % 3) * 2) / 1000;
      const int sy = i % 2 ? top - rise : top + height + rise;
      r.disk(sx, sy, 3 - age * 2 / 1000, i % 3 ? kGlow : kFullTop);
    }
  }
  if (bar.gulp) {
    // A flash along what's filled, and sparks off the tip.
    r.alpha = shown * (1000 - bar.gulp) / 1000 / 2;
    if (tip > x + 4) r.round(x + 2, top + 2, tip - x - 2, height - 4, (height - 4) / 2, kGlow);
    r.alpha = shown * (1000 - bar.gulp) / 1000;
    for (int i = 0; i < 6; ++i) {
      const Point out = turn(i * 32 / 6 + 3);
      const int d = 8 + bar.gulp * 26 / 1000;
      r.disk(tip + d * out.x / 4096, cy + d * out.y / 4096, 2, i % 2 ? kGlow : kHeart);
    }
  }
  if (bar.flight >= 0) {
    // Up and over from the bubble, quicker as the bar draws it in: a curve
    // that starts out and to the side, and three fading behind it.
    const Point centre = wishCentre(s, at), from{centre.x, centre.y - 6};  // where its heart sat
    const auto along = [&](int k, int& px, int& py) {
      const int e = k * k / 1000, q = 1000 - e;
      // Over anything under the bar (a LISTENING word), along its row.
      const int midX = from.x + 60, midY = cy;
      px = (q * q / 1000 * from.x + 2 * q * e / 1000 * midX + e * e / 1000 * tip) / 1000;
      py = (q * q / 1000 * from.y + 2 * q * e / 1000 * midY + e * e / 1000 * cy) / 1000;
    };
    for (int i = 3; i >= 0; --i) {
      const int k = std::max(0, bar.flight - i * 45);
      int px, py;
      along(k, px, py);
      r.alpha = i ? 200 - i * 50 : 255;
      heart(r, px * 16, py * 16, std::max(6, 20 - k * 10 / 1000 - i * 3), kHeart);
    }
  }
  r.alpha = 255;
}

int signHop(const SignShow& sign, const BodySettings& body) {
  if (!sign.text || sign.low || body.size >= kChonkBody.size) return 0;
  const int most = body.size >= kBuddyBody.size
                       ? SignLayer::kBuddyHopPx * (kChonkBody.size - body.size) / (kChonkBody.size - kBuddyBody.size)
                       : SignLayer::kBuddyHopPx + (SignLayer::kHopPx - SignLayer::kBuddyHopPx) * (kBuddyBody.size - body.size) /
                                                      (kBuddyBody.size - kHatchlingBody.size);
  const int in = std::min<int>(sign.ms, kSignRiseMs) * 1000 / int(kSignRiseMs);
  return most * ease1000(in, 1000) / 1000 * sign.opacity / 1000;
}

void paintSign(Surface& s, const SignShow& sign, const rig::Anchors& at, int hop) {
  if (!s.pixels || !sign.text) return;
  Raster r{s};
  r.fade = (sign.opacity * 255 + 500) / 1000;
  const int bottom = s.height * 4 / 5;  // the lane's top: shapes stop there
  const Font& f = kSign;
  // Never narrower than kMinWide, so a short one ("<3") isn't smaller than
  // the hands that hold it.
  const int tw = stringWidth(f, sign.text), w = std::max(tw + 2 * SignLayer::kPad, SignLayer::kMinWide), h = f.h + 10;
  // Centred on its front, held at its hands' ring, never into the lane,
  // nor past the panel's sides.
  // A card held under the mouth, which is the deepest an open one reaches, so
  // nothing of it pokes above the card.
  // (A wide laugh's open mouth reaches a sixth of the eyes' spacing under its
  // middle.)
  const int underMouth = (at.mouth.y + (at.eyeR.x - at.eyeL.x) / 6) / 16 + hop;
  const int y = std::min(std::max(at.handF.y / 16 + hop - h / 2, underMouth), bottom - SignLayer::kGap - h) +
                signLift(sign.ms, SignLayer::kRisePx, SignLayer::kBobPx);
  // (A hand's disk takes the kHand pixels either side of its middle's.)
  const int edge = s.inset(y, y + h) + SignLayer::kEdge + SignLayer::kHand, most = s.width - 1 - edge - w;
  const int x = most < edge ? (s.width - w) / 2 : std::clamp(at.handF.x / 16 - w / 2, edge, most);
  r.round(x, y, w, h, 12, kCardInk);
  r.ring(x, y, w, h, 12, 2, kCardRim);
  text(r, f, x + (w - tw) / 2, y + 5, sign.text, kWords);
  // A little gel hand at either side, gripping it.
  for (int side : {-1, 1}) {
    const int hx = side < 0 ? x : x + w, hy = y + h / 2;
    r.disk(hx, hy, SignLayer::kHand, kGel);
    r.alpha = 160;
    r.disk(hx - 4, hy - 5, 3, kShine);
    r.alpha = 255;
  }
}

}  // namespace render::gel
