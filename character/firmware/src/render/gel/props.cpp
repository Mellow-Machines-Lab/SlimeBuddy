#include "render/gel/painter.h"
#include "render/gel/engine.h"
#include "render/gel/pastimes.h"
#include "render/gel/raster.h"
#include "render/gel/subpixel.h"
#include "render/tags.h"
namespace render::gel {
namespace {
// The cards, in thousandths of the surface: x, y, width, height, radius.
struct Card {
  int x, y, w, h, radius;
};
constexpr Card kAlertCard{180, 75, 640, 215, 32}, kStartCard{280, 85, 440, 132, 30};
// A card on a surface, in pixels, with its corners' radius as Raster::round
// clamps it.
struct Placed {
  int x, y, w, h, r;
};
Placed place(const Surface& surface, const Card& c) {
  const int w = surface.width * c.w / 1000, h = surface.height * c.h / 1000;
  return {surface.width * c.x / 1000, surface.height * c.y / 1000, w, h,
          std::clamp(surface.width * c.radius / 1000, 0, std::min(w, h) / 2)};
}
// Raster::round fills at the card's opacity: opaque from 253 on.
bool opaque(F visible) { return (visible * 255).integer() >= 253; }

// What the cards show, as a key that's the same exactly when their pixels
// are (never 0), when one card shows, opaque: 0 while one fades, and when
// two overlap, since the one on top covers the other's corners.
uint32_t cardsKey(const Scenes& s, T now) {
  uint32_t h = 2166136261u;
  auto add = [&](const void* data, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const uint8_t*>(data)[i]) * 16777619u;
  };
  auto str = [&](const char* v) { add(v ? v : "", v ? std::strlen(v) + 1 : 1); };
  int shown = 0;
  for (auto prop : {Scenes::kStartProp, Scenes::kAlertProp}) {
    const F seen = s.visible(prop, now);
    if (seen <= F(0)) continue;
    if (!opaque(seen) || ++shown > 1) return 0;
    // Its labels' and marks' soft edges still blend at 253 and 254.
    const int facts[] = {int(prop), (seen * 255).integer()};
    add(facts, sizeof facts);
    if (prop == Scenes::kStartProp) str(s.startProp.label);
  }
  return shown ? (h ? h : 1) : 0;
}

// The card's first row something's been drawn over yet this frame, or the
// row under it.
int firstDrawnRow(const Surface& surface, const Placed& c) {
  for (int y = std::max(0, c.y); y < std::min(surface.height, c.y + c.h); ++y) {
    const auto& row = surface.written[y];
    if (row.last && row.first < c.x + c.w && row.last > c.x) return y;
  }
  return c.y + c.h;
}
}  // namespace

int tapLift(const Scenes& s, int side) {
  const int age = F(s.ageMs).integer();
  for (int i = 0; i < s.recipe.count; ++i) {
    const auto& k = s.recipe.contacts[i];
    if (k.kind != kKey || k.side != side) continue;
    const int d = F(k.at).integer() - age;
    if (d >= 0 && d < kTapMs) return kTapLift * 4 * d * (kTapMs - d) / (kTapMs * kTapMs);
  }
  return 0;
}

int typeLift(uint32_t ms, int side) {
  // Each paw taps every other kTypeMs, the right's between the left's.
  const int period = 2 * kTypeMs, p = int((ms + (side > 0 ? kTypeMs : 0)) % period), d = (period - p) % period;
  return d < kTapMs ? kTypeLift * 4 * d * (kTapMs - d) / (kTapMs * kTapMs) : 0;
}

int opaqueCards(const Surface& surface, const Engine& e, Box* out) {
  const auto& s = e.scenes;
  const T now = e.timeMs();
  const int w = surface.width, bottom = surface.height * 4 / 5;
  int count = 0;
  auto add = [&](const Card& card, Scenes::Prop prop) {
    if (!opaque(s.visible(prop, now))) return;
    const auto [x, y, cw, ch, r] = place(surface, card);
    // Its whole rows between the corners, and the columns between them above and below.
    const Box boxes[] = {
        {x + r, y, x + cw - r, y + r}, {x, y + r, x + cw, y + ch - r}, {x + r, y + ch - r, x + cw - r, y + ch}};
    for (Box b : boxes) {
      b.x0 = std::max(b.x0, 0), b.x1 = std::min(b.x1, w), b.y0 = std::max(b.y0, 0), b.y1 = std::min(b.y1, bottom);
      if (b.x0 < b.x1 && b.y0 < b.y1 && count < Surface::kMaxKeep) out[count++] = b;
    }
  };
  add(kAlertCard, Scenes::kAlertProp);
  add(kStartCard, Scenes::kStartProp);
  return count;
}

void paintProps(Surface& surface, const Engine& e, const Paws* paws) {
  if (!surface.pixels) return;
  Raster r{surface};
  auto& s = e.scenes;
  auto state = s.route.state;
  int w = surface.width, h = surface.height;
  auto x = [&](int p) { return w * p / 1000; };
  auto y = [&](int p) { return h * p / 1000; };
  auto round = [&](int a, int b, int c, int d, int radius, uint16_t ink) {
    r.round(x(a), y(b), x(c), y(d), x(radius), ink);
  };
  // A card, filled, and its one-pixel rim, a step lighter.
  auto fillCard = [&](const Card& c, uint16_t ink, uint16_t rim) {
    round(c.x, c.y, c.w, c.h, c.radius, ink);
    r.ring(x(c.x), y(c.y), x(c.w), y(c.h), x(c.radius), 1, rim);
  };
  // A card, and what's drawn on it, cut to the card. One this surface
  // already shows (cardKey) keeps its rows above the first that anything's
  // been drawn over yet this frame, or will be (`over`, a knock), or was
  // when the surface last showed it (cardKeyTo): the clear kept their
  // insides (opaqueCards), so only their corners, which it erased, are
  // drawn again. From that row down it's drawn whole. Drawing needs you's
  // again every frame cost 2.3 ms on the board.
  const uint32_t cards = cardsKey(s, e.timeMs());
  int cardsTo = 1 << 30;
  Box drawn[Surface::kMaxCards];
  int drawnCount = 0;
  auto drawCard = [&](const Card& card, uint16_t ink, uint16_t rim, auto&& on, int over = 1 << 30) {
    const Placed c = place(surface, card);
    if (drawnCount < Surface::kMaxCards) drawn[drawnCount++] = {c.x, c.y, c.x + c.w, c.y + c.h};
    const int from =
        cards && cards == surface.cardKey ? std::min({firstDrawnRow(surface, c), surface.cardKeyTo, over}) : c.y;
    cardsTo = over;
    auto within = [&](Box b) {
      r.bounds = b;
      fillCard(card, ink, rim);
      on();
    };
    if (from > c.y) {
      r.touch(c.x, c.y, c.x + c.w, from);
      for (int top : {c.y, c.y + c.h - c.r})
        for (int left : {c.x, c.x + c.w - c.r}) within({left, top, left + c.r, std::min(top + c.r, from)});
    }
    within({c.x, from, c.x + c.w, c.y + c.h});
    r.bounds = Raster::kEverywhere;
  };
  auto text = [&](const char* label, int a, int b, uint16_t ink, const Font& font = kSmall) {
    r.text(label, x(a), y(b), ink, font);
  };
  const uint16_t dark = rgb565(20, 55, 72), green = rgb565(140, 223, 203), light = rgb565(184, 233, 245),
                 blue = rgb565(100, 184, 207), amber = rgb565(239, 189, 103);
  F contact = s.envelope(e.timeMs(), kKey), progress = clamp(s.ageMs / s.recipe.duration, 0, 1);
  if (s.desk.x > q(.001)) {
    r.alpha = (s.desk.x * 255).integer();
    round(150, 749, 700, 21, 10, rgb565(25, 59, 77));
    round(100, 600, 184, 116, 12, blue);
    round(104, 604, 176, 108, 10, dark);
    round(114, 611, 156, 89, 8, rgb565(7, 24, 32));
    round(178, 715, 26, 32, 8, dark);
    round(140, 739, 98, 10, 5, blue);
    auto deskState = isDesk(state) ? state : s.route.baseState;
    if (deskState == SceneState::kPlanning) {
      for (int i = 0; i < 3; ++i) {
        round(130, 627 + i * 23, 10, 8, 3, green);
        round(150, 628 + i * 23, 63 + i * 12, 4, 2, blue);
      }
    } else if (deskState == SceneState::kSearching) {
      int sx = 165 + (progress * 32).integer();
      r.disk(x(sx), y(645), 8, green);
      r.disk(x(sx), y(645), 5, dark);
      r.line({x(sx + 13), y(658)}, {x(sx + 28), y(675)}, 2, green);
    } else if (deskState == SceneState::kTesting) {
      for (int i = 0; i < 4; ++i) r.disk(x(143 + i * 31), y(644), 3, green);
      text("TEST", 191, 678, blue);
    } else if (deskState == SceneState::kAnalyzing) {
      round(135, 627, 38, 52, 4, blue);
      round(190, 627, 38, 52, 4, blue);
      round(139 + (progress * 55).integer(), 648, 30, 5, 2, green);
    } else if (deskState == SceneState::kToolUse) {
      r.disk(x(178), y(647), 7, green);
      r.disk(x(178), y(647), 4, dark);
      r.line({x(194), y(647)}, {x(219), y(647)}, 2, green);
      r.line({x(219), y(647)}, {x(219), y(658)}, 2, green);
    } else if (deskState == SceneState::kWaiting)
      text(". . .", 191, 647, blue);
    else {
      text(deskState == SceneState::kTerminal ? "> _" : ". . .", 162, 633, green);
      for (int i = 0; i < 3; ++i) round(129, 654 + i * 13, 100 - i * 24, 3, 1, blue);
    }
    // The keyboard and the paws are its size, about the desk's front under it.
    const int size = e.body().size;
    // They go as it melts, which would leave them lying across its face.
    const int kit = (s.desk.x * 255).integer() * (4096 - (paws ? paws->melted : 0)) / 4096;
    r.alpha = kit;
    auto scaled = [&](int v) { return v * size / 4096; };
    auto atX = [&](int v) { return 500 + scaled(v - 500); };
    auto atY = [&](int v) { return 749 + scaled(v - 749); };
    round(atX(387), atY(713), scaled(258), scaled(30), scaled(8), rgb565(49, 88, 107));
    for (int row = 0; row < 2; ++row)
      for (int key = 0; key < 9; ++key) {
        bool pressed = contact > q(.35) && key == (s.hasLast && s.last.side < 0 ? 2 : 6);
        round(atX(397 + key * 26), atY(718 + row * 10), scaled(19), scaled(6), 2, pressed ? light : rgb565(144, 170, 176));
      }
    for (int side : {-1, 1}) {
      const int i = side > 0, alpha = paws ? paws->alpha[i] : 255;
      if (alpha < 3) continue;
      const int lift = std::max(tapLift(s, side), typeLift(uint32_t(F(e.timeMs()).integer()), side)) +
                       (paws ? paws->lift[i] : 0);
      int yy = 711 + (s.hasLast && s.last.side == side ? (contact * 9).integer() : 0) - lift;
      r.alpha = kit * alpha / 255;
      round(atX(500 + side * 78 - 30 + (paws ? paws->dx[i] : 0)), atY(yy - 12), scaled(60), scaled(25), scaled(13),
            rgb565(93, 166, 219));
    }
  }
  // Each prop's alpha is its ramped visibility; one fading out still draws
  // the content it last showed.
  const T now = e.timeMs();
  r.alpha = 255;
  if (F listening = s.visible(Scenes::kListeningProp, now); listening > F(0)) {
    r.alpha = (listening * 255).integer();
    // The mic and its label in a row over the head, centred together, the
    // label in the large font to read at arm's length, as the cards' do,
    // and clear of the dome's top. The ring breathes in sixteenths of a
    // pixel, two pixels wide, its outer edge 15.5 to 21.5 px out.
    constexpr int kMic = 372, kLabel = 555;
    circle16(r, x(kMic) * 16 + 8, y(144) * 16 + 8, 15 * 16 - 8 + (s.listeningPulse * 96).integer(), 2 * 16, blue);
    round(kMic - 8, 119, 16, 31, 8, light);
    r.line({x(kMic), y(153)}, {x(kMic), y(164)}, 2, blue);
    text("LISTENING", kLabel, 144, light, kLarge);
  }
  if (F start = s.visible(Scenes::kStartProp, now); start > F(0)) {
    r.alpha = (start * 255).integer();
    drawCard(kStartCard, rgb565(27, 55, 78), rgb565(0x78, 0xa4, 0xc8),
             [&] { text(s.startProp.label, 500, 151, light, kLarge); });
  }
  if (F touch = s.visible(Scenes::kTouchProp, now); touch > F(0)) {
    F pulse = s.envelope(now, kBody);
    r.alpha = (touch * pulse * 255).integer();
    for (int i = 0; i < (s.touchKind == SceneState::kTapSpam ? 3 : 1); ++i)
      r.line({x(798 + i * 16), y(462)}, {x(820 + i * 16), y(480)}, 2, light);
  }
  r.alpha = 255;
  if (F alert = s.visible(Scenes::kAlertProp, now); alert > F(0)) {
    r.alpha = (alert * 255).integer();
    // A knock's disk reaches a row above its radius (Raster::disk).
    const F knock = state == SceneState::kNeedsYou ? s.envelope(now, kKnock) : F(0);
    const int knockY = y(246), knockRadius = 6 + (knock * 4).integer();
    drawCard(
        kAlertCard, amber, rgb565(0xf8, 0xd2, 0x94),
        [&] {
          // Sized to read at arm's length on the 2.06" panel.
          text("NEEDS YOU", 500, 133, rgb565(44, 37, 29), kSign);
          text("Continue on your Mac", 500, 227, rgb565(0x3d, 0x2f, 0x1f), kLarge);
        },
        knock > 0 ? knockY - knockRadius - 1 : 1 << 30);
    if (knock > 0) {
      r.alpha = (knock * 255).integer();
      r.disk(x(280), knockY, knockRadius, light);
    }
  }
  // What's drawn over the cards after them, as the frame goes on, lowers
  // cardKeyTo (Surface::touch).
  surface.cardKey = cards;
  surface.cardKeyTo = cardsTo;
  std::copy(drawn, drawn + drawnCount, surface.cardBoxes);
  surface.cardBoxCount = drawnCount;
}
void paintSprout(Surface& surface, const Engine& e, const rig::Anchors& at, int scale) {
  if (!surface.pixels || e.bodyChange.stage != 2 || !e.bodyChange.arrived(e.elapsedMs) ||
      wornHat(e.scenes.when) != Outfit::kNone)
    return;
  Raster r{surface};
  const uint16_t stem = rgb565(92, 190, 96), leaf = rgb565(128, 226, 122);
  const auto px = [&](int v) { return v * scale / 4096; };
  const int x = at.crown.x / 16, y = at.crown.y / 16 + 3, top = y - px(38);
  r.line({x, y}, {x, top}, std::max(2, px(5)), stem);
  // Each leaf a run of dots, widest at its middle, out and a little up.
  for (int side : {-1, 1})
    for (int i = 1; i <= 5; ++i) {
      const int across = side * px(7) * i, up = px(i * (10 - i)) * 3 / 4;
      r.disk(x + across, top + px(10) - up, std::max(1, px(9 - std::abs(3 - i) * 3)), leaf);
    }
}
namespace {
// Shapes stop at the lane: what's going to reach it fades over this many rows
// first, so nothing is cut flat in mid-air.
constexpr int kLaneFadeRows = 72, kLaneFadeBands = 9;
// The share of `alpha` left `y16` down, ending at the lane.
int laneFade(const Surface& s, int y16, int alpha) {
  const int left = s.height * 4 / 5 * 16 - y16;
  return left >= kLaneFadeRows * 16 ? alpha : std::max(0, alpha * left / (kLaneFadeRows * 16));
}
// A ring round the body, from the top to where the lane fades it away.
void ringToTheLane(Raster& r, int cx, int cy, int radius, int width, uint16_t ink, int alpha) {
  const int stop = r.s.height * 4 / 5, from = stop - kLaneFadeRows, band = kLaneFadeRows / kLaneFadeBands;
  r.alpha = alpha;
  r.bounds = Box{0, 0, r.s.width, from};
  circle16(r, cx, cy, radius, width, ink);
  for (int i = 0; i < kLaneFadeBands; ++i) {
    r.alpha = alpha * (kLaneFadeBands - i) / (kLaneFadeBands + 1);
    r.bounds = Box{0, from + i * band, r.s.width, from + (i + 1) * band};
    circle16(r, cx, cy, radius, width, ink);
  }
  r.bounds = Raster::kEverywhere;
}
}  // namespace
void paintGrowing(Surface& surface, const Engine& e, const rig::Anchors& at) {
  if (!surface.pixels) return;
  const uint32_t now = e.elapsedMs;
  const int glow = e.bodyChange.glow(now), burst = e.bodyChange.burst(now);
  if (!glow && !burst) return;
  Raster r{surface};
  const uint16_t warm = rgb565(255, 232, 150), bright = rgb565(255, 250, 220);
  // Round the body as it is this frame, so no ring crosses it: its middle,
  // and far enough to clear its widest and its crown.
  int left = at.hem[0].x, right = at.hem[0].x;
  for (const rig::Point& p : at.hem) left = std::min(left, p.x), right = std::max(right, p.x);
  for (const rig::Point& p : at.lap) left = std::min(left, p.x), right = std::max(right, p.x);
  const int low = at.hem[rig::Anchors::kRingPoints / 2].y, cx = at.crown.x, cy = (at.crown.y + low) / 2;
  const int reach = std::max(160, std::max(right - left, low - at.crown.y) * 9 / 16);
  // The glow: rings close round it, brighter as it draws in.
  if (glow)
    for (int i = 0; i < 3; ++i) ringToTheLane(r, cx, cy, reach + (6 + i * 12) * 16, 3 * 16, warm, glow * (3 - i) / 4);
  // The burst: a ring and sparks flying out, fading.
  if (burst) {
    static constexpr int8_t kWays[10][2] = {{100, 0},  {81, 59},  {31, 95},  {-31, 95},  {-81, 59},
                                            {-100, 0}, {-81, -59}, {-31, -95}, {31, -95}, {81, -59}};
    const int out = reach + burst * 120 * 16 / 255, fade = 255 - burst;
    ringToTheLane(r, cx, cy, out, 4 * 16, bright, fade);
    for (const auto& way : kWays) {
      const int y16 = cy + way[1] * out / 100;
      r.alpha = laneFade(surface, y16, fade);
      r.disk((cx + way[0] * out / 100) / 16, y16 / 16, 5 - burst * 3 / 255, bright);
    }
  }
  r.alpha = 255;
}
}  // namespace render::gel
