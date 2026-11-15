#include "render/gel/painter.h"
#include "render/gel/engine.h"
#include "render/gel/pastimes.h"
#include "render/gel/raster.h"
#include "render/gel/subpixel.h"
#include "render/gel/lane.h"
#include "render/tags.h"
namespace render::gel {
namespace {
// What a request asks of you, as the needs-you card says it after
// "NEEDS YOU": short enough for its second line (16 glyphs at most).
const char* attnKindWords(AttnKind k) {
  switch (k) {
    case AttnKind::kApprove: return "a yes or no";
    case AttnKind::kQuestion: return "a question";
    case AttnKind::kPlan: return "a plan to read";
    case AttnKind::kNone: break;
  }
  return "";
}
// The cards, in thousandths of the surface: x, y, width, height, radius.
struct Card {
  int x, y, w, h, radius;
};
// The needs-you card stands in the top band, 344 × 76 px on the AMOLED from
// y 16, clear of the crown, which ducks under it (paintFrame's duck).
constexpr Card kAlertCard{158, 40, 685, 186, 40};
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

// What the needs-you card shows, as a key that's the same exactly when its
// pixels are (never 0), while it shows, opaque: 0 while it fades.
uint32_t cardsKey(const Scenes& s, T now) {
  const F seen = s.visible(Scenes::kAlertProp, now);
  if (seen <= F(0) || !opaque(seen)) return 0;
  // Its labels' and marks' soft edges still blend at 253 and 254.
  uint32_t h = 2166136261u;
  const int facts[] = {int(Scenes::kAlertProp), (seen * 255).integer()};
  for (size_t i = 0; i < sizeof facts; ++i) h = (h ^ reinterpret_cast<const uint8_t*>(facts)[i]) * 16777619u;
  return h ? h : 1;
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
                 blue = rgb565(100, 184, 207);
  F contact = s.envelope(e.timeMs(), kKey);
  if (s.desk.x > q(.001)) {
    r.alpha = (s.desk.x * 255).integer();
    round(150, 749, 700, 21, 10, rgb565(25, 59, 77));
    round(100, 600, 184, 116, 12, blue);
    round(104, 604, 176, 108, 10, dark);
    round(114, 611, 156, 89, 8, rgb565(7, 24, 32));
    round(178, 715, 26, 32, 8, dark);
    round(140, 739, 98, 10, 5, blue);
    // The monitor says the activity in one shape that fills it: lines of
    // text growing as it types, a lit line stepping down a page as it
    // looks, one bar filling as something runs, three dots as it waits.
    const auto deskState = isDesk(state) ? state : s.route.baseState;
    const uint32_t ms = uint32_t(F(e.timeMs()).integer());
    const uint16_t dim = rgb565(38, 84, 104);
    switch (activityOf(deskState)) {
      case Activity::kTyping: {
        // Three lines, each typed out in steps over a second, then cleared.
        const int at = int(ms % 3800);
        for (int i = 0; i < 3; ++i) {
          const int full = 132 - i * 22, typed = std::clamp((at - i * 1000) * 11 / 900, 0, 11) * full / 11;
          if (typed > 0) round(126, 622 + i * 26, typed, 12, 5, at < (i + 1) * 1000 ? green : blue);
        }
        break;
      }
      case Activity::kLooking: {
        // A page of four lines, the one being read lit.
        const int lit = int(ms / 700 % 4);
        for (int i = 0; i < 4; ++i)
          round(126, 619 + i * 20, i == lit ? 132 : 132 - (i * 37 % 50), i == lit ? 12 : 9, 4, i == lit ? light : dim);
        break;
      }
      case Activity::kRunning: {
        // One bar, filling over 2.4 s, full for a beat, and again.
        const int filled = std::min(int(ms % 3000), 2400) * 124 / 2400;
        round(122, 634, 140, 44, 10, dim);
        round(126, 640, 132, 32, 7, rgb565(7, 24, 32));
        if (filled > 8) round(130, 646, filled, 20, 4, green);
        break;
      }
      case Activity::kWaiting: {
        // Three dots, each brighter in its turn, slowly.
        const int turn = int(ms / 600 % 4);
        for (int i = 0; i < 3; ++i) r.disk(x(152 + i * 40), y(655), 5, i == turn ? light : blue);
        break;
      }
    }
    // The keyboard and the paws go as it melts, which would leave them lying across its face.
    r.alpha = (s.desk.x * 255).integer() * (4096 - (paws ? paws->melted : 0)) / 4096;
    round(387, 713, 258, 30, 8, rgb565(49, 88, 107));
    for (int row = 0; row < 2; ++row)
      for (int key = 0; key < 9; ++key) {
        bool pressed = contact > q(.35) && key == (s.hasLast && s.last.side < 0 ? 2 : 6);
        round(397 + key * 26, 718 + row * 10, 19, 6, 2, pressed ? light : rgb565(144, 170, 176));
      }
    // The paws, by the activity, easing from one pose to the next
    // (Scenes::pose): on the keys and typing; off the keys and still on the
    // desk, to the monitor's side, as it looks; lifted and held together in
    // front as something runs; flat on the keys as it waits.
    const F looking = s.posed(Activity::kLooking), running = s.posed(Activity::kRunning),
            waiting = s.posed(Activity::kWaiting), typing = clamp(F(1) - looking - running - waiting, 0, 1);
    for (int side : {-1, 1}) {
      const int i = side > 0;
      const int taps = (typing * std::max(tapLift(s, side), typeLift(ms, side))).integer();
      const int lift =
          taps + (paws ? paws->lift[i] : 0) +
          (looking * kLookingPaws.lift[i] + running * kRunningPaws.lift[i] + waiting * kWaitingPaws.lift[i]).integer();
      const int dx =
          (paws ? paws->dx[i] : 0) +
          (looking * kLookingPaws.dx[i] + running * kRunningPaws.dx[i] + waiting * kWaitingPaws.dx[i]).integer();
      int yy = 711 + (s.hasLast && s.last.side == side ? (contact * 9).integer() : 0) - lift;
      round(500 + side * 78 - 30 + dx, yy - 12, 60, 25, 13, rgb565(93, 166, 219));
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
  if (F touch = s.visible(Scenes::kTouchProp, now); touch > F(0)) {
    F pulse = s.envelope(now, kBody);
    r.alpha = (touch * pulse * 255).integer();
    for (int i = 0; i < (s.touchKind == SceneState::kTapSpam ? 3 : 1); ++i)
      r.line({x(798 + i * 16), y(462)}, {x(820 + i * 16), y(480)}, 2, light);
  }
  r.alpha = 255;
  if (F alert = s.visible(Scenes::kAlertProp, now); alert > F(0)) {
    r.alpha = (alert * 255).integer();
    // A knock's ring round the kind's mark reaches a row above its radius.
    const F knock = state == SceneState::kNeedsYou ? s.envelope(now, kKnock) : F(0);
    const Placed c = place(surface, kAlertCard);
    const AttnKind kind = e.router.facts.attention.kind;
    const uint16_t fill = rgb565(kAmberRgb), ink = rgb565(kNeedsYouInkRgb), soft = mix565(fill, ink, 204);
    // The kind's mark in its column on the left, the two lines after it,
    // left-aligned, so the card scans as a mark and two lines; with no kind
    // the two centred, the second the old one.
    const int markX = c.x + 36, markY = c.y + c.h / 2, lines = c.x + 64;
    const int knockRadius = 22 + (knock * 5).integer();
    drawCard(
        kAlertCard, fill, fill,
        [&] {
          const int top = c.y + 4, second = top + 30;
          if (kind == AttnKind::kNone) {
            const char* more = "Continue on your Mac";
            render::gel::text(r, kSign, c.x + (c.w - 9 * kSign.w) / 2, top, "NEEDS YOU", ink);
            render::gel::text(r, kLarge, c.x + (c.w - glyphCount(more) * kLarge.w) / 2, second + 4, more, soft);
            return;
          }
          if (kind == AttnKind::kPlan) {
            // A page, three lines on it.
            r.round(markX - 14, markY - 18, 28, 36, 4, ink);
            for (int i = 0; i < 3; ++i) r.round(markX - 9, markY - 10 + i * 8, i == 2 ? 11 : 18, 3, 1, fill);
          } else {
            r.disk(markX, markY, 18, ink);
            if (kind == AttnKind::kQuestion) {
              render::gel::text(r, kSign, markX - kSign.w / 2, markY - 25, "?", fill);
            } else {
              // A yes and a no, split: a tick on the left half, a cross on the right.
              r.line({markX - 12, markY}, {markX - 8, markY + 5}, 3, fill);
              r.line({markX - 8, markY + 5}, {markX - 2, markY - 6}, 3, fill);
              r.line({markX + 3, markY - 5}, {markX + 11, markY + 5}, 3, fill);
              r.line({markX + 11, markY - 5}, {markX + 3, markY + 5}, 3, fill);
            }
          }
          render::gel::text(r, kSign, lines, top, "NEEDS YOU", ink);
          render::gel::text(r, kSign, lines, second, attnKindWords(kind), soft, c.x + c.w - 12 - lines);
        },
        knock > 0 && kind != AttnKind::kNone ? markY - knockRadius - 1 : 1 << 30);
    if (knock > 0 && kind != AttnKind::kNone) {
      r.alpha = (knock * 255).integer();
      circle16(r, markX * 16 + 8, markY * 16 + 8, knockRadius * 16, 2 * 16, ink);
    }
  }
  // What's drawn over the cards after them, as the frame goes on, lowers
  // cardKeyTo (Surface::touch).
  surface.cardKey = cards;
  surface.cardKeyTo = cardsTo;
  std::copy(drawn, drawn + drawnCount, surface.cardBoxes);
  surface.cardBoxCount = drawnCount;
}
}  // namespace render::gel
