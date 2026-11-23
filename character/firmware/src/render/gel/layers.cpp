#include "render/gel/layers.h"

#include <algorithm>
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
// The pale gold of a wish's sparks.
constexpr uint16_t kMoonlight = rgb565(250, 230, 160);

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

// A triangle wave, 0 to `amp` and back, once every `period` ms.
int wave(int ms, int period, int amp) {
  const int p = ms % period;
  return (p < period / 2 ? p : period - p) * 2 * amp / period;
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

// Listening to you speak: a gel hand cupped at its ear on `side` (-1 left,
// 1 right), and three rings beside it, each swelling out and fading on the
// 1.6 s beat of its nod, `ms` into it.
void speakingHand(Raster& r, const rig::Anchors& at, int side, int ms) {
  const int ear = side > 0 ? at.brimR.x : at.brimL.x, earY = (at.brimF.y + at.eyeL.y) / 2;
  const int hx = ear / 16 + side * 14, hy = earY / 16;
  r.alpha = 255;
  // Rimmed dark, so it reads apart from the body it's held against.
  r.disk(hx, hy, 18, kCardInk);
  r.disk(hx + side * 8, hy - 10, 9, kCardInk);
  r.disk(hx, hy, 15, kGel);
  r.disk(hx + side * 8, hy - 10, 6, kGel);  // the fingers, curled forward
  r.disk(hx - 5, hy - 6, 3, kShine);
  const int beat = ms % 1600;
  for (int i = 0; i < 3; ++i) {
    const int age = (beat + 1600 - i * 330) % 1600, radius = 26 + i * 11 + age * 12 / 1600;
    r.alpha = std::max(0, 255 - age * 255 / 1600);
    // A third of a turn about the hand, facing out: turn() counts
    // thirty-seconds clockwise from the right.
    Point arc[11];
    for (int k = 0; k < 11; ++k) {
      const Point p = turn((side > 0 ? 32 : 16) + k - 5);
      arc[k] = {hx * 16 + radius * p.x / 256, hy * 16 + radius * p.y / 256};
    }
    r.stroke16(arc, 11, 3 * 16, kShine);
  }
  r.alpha = 255;
}

}  // namespace

int sleepyLids(const Engine& e) {
  if (e.energy != Energy::kTired) return 0;
  // They go as the face squashes shut, gone by when it's a line, and come
  // back as it wakes.
  const F open = max(0, 1 - e.face.shut.x / SemanticFace::kShutLine);
  return int((open.raw * kNightLids + F::scale / 2) / F::scale);
}

Contents layerContents(const Engine& e) {
  const bool zs = hasTag(e.scenes.when, "night") && e.scenes.route.state == SceneState::kAsleep;
  return {zs ? kZs : 0u};
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
  if (e.speaking) speakingHand(r, at, e.speakingSide, ms);
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
  // met turns white once its heart has popped.
  const int goes = wish.pop == WishShow::kGone ? 0 : pop / 2;
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

int signHop(const SignShow& sign) {
  if (!sign.text || sign.low) return 0;
  const int in = std::min<int>(sign.ms, kSignRiseMs) * 1000 / int(kSignRiseMs);
  return SignLayer::kHopPx * ease1000(in, 1000) / 1000 * sign.opacity / 1000;
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
