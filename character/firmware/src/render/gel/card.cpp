#include "render/gel/card.h"

#include <algorithm>

#include "render/font.h"
#include "render/gel/lane.h"
#include "render/gel/raster.h"
#include "render/maths.h"
#include "render/palette.h"
#include "render/stream.h"

namespace render::gel {

namespace {
constexpr uint16_t kCardInk = rgb565(16, 20, 24), kBarWords = rgb565(10, 30, 34), kCardRim = rgb565(0x78, 0xa4, 0xc8), kWords = rgb565(232, 242, 250),
                   kSmallWords = rgb565(143, 166, 184), kOver = rgb565(255, 122, 122), kGel = rgb565(89, 169, 224),
                   kShine = rgb565(184, 233, 245), kDotOff = rgb565(58, 74, 90);
const Font& kMain = kSign;
const Font& kSmallFont = kLarge;

bool hasSmall(const CardShow& c) { return c.small && c.small[0]; }
// The agent's title bar, as a window's: its name in the large font on its
// lane's colour, so the card reads as the agent's and never as one of
// Boop's own round, glowing signs. None with no agent named.
bool hasBar(const CardShow& c) { return c.agent && c.agent[0]; }
// With a bar the words sit closer in, so the card isn't much taller and
// Boop needn't stand much higher on it.
constexpr int kBar = 24, kCorner = 5;
int pad(const CardShow& c) { return hasBar(c) ? 2 : 6; }
int dotsRow(const CardShow& c) { return c.count > 1 ? CardLayout::kDotsRow : 0; }
}  // namespace

int cardHeight(const CardShow& c) { return (hasBar(c) ? kBar : 0) + pad(c) + kMain.h + (hasSmall(c) ? kSmallFont.h : 0) + pad(c); }

int cardHop(const CardShow& c) {
  if (!c.on) return 0;
  // Its mouth is this far over the lane at rest.
  constexpr int room = 62;
  const int needs = cardHeight(c) + dotsRow(c) + CardLayout::kGap + 8;
  const int in = int(std::min<uint32_t>(c.ms, CardLayout::kRiseMs)) * 1000 / int(CardLayout::kRiseMs);
  return std::max(0, needs - room) * ease1000(in, 1000) / 1000;
}

Box cardBox(const Surface& s, const CardShow& c) {
  const int h = cardHeight(c), bottom = s.height * 4 / 5 - CardLayout::kGap - dotsRow(c), y = bottom - h;
  const int widest = std::max(stringWidth(kMain, c.text), hasSmall(c) ? stringWidth(kSmallFont, c.small) : 0);
  // (The bar's name is short, `claude` or `codex`: the main line is wider.)
  const int edge = s.inset(y, y + h) + CardLayout::kEdge + CardLayout::kHand;
  const int w = std::clamp(widest + 2 * CardLayout::kPad, CardLayout::kMinWide, s.width - 2 * edge);
  const int x = (s.width - w) / 2;
  return {x, y, x + w, y + h};
}

void paintCard(Surface& s, const CardShow& c) {
  if (!s.pixels || !c.on) return;
  Raster r{s};
  const int in = int(std::min<uint32_t>(c.ms, CardLayout::kRiseMs)) * 1000 / int(CardLayout::kRiseMs);
  const int eased = ease1000(in, 1000);
  r.fade = eased * 255 / 1000;
  Box b = cardBox(s, c);
  const int down = CardLayout::kRisePx * (1000 - eased) / 1000, w = b.x1 - b.x0, h = b.y1 - b.y0;
  const int x = b.x0, y = b.y0 + down;
  const uint16_t rim = c.slot >= 0 && c.slot < Stream::kLanes ? kLaneInk[c.slot] : kCardRim;
  // A window's corners, square-ish, against the round signs Boop holds up.
  r.round(x, y, w, h, kCorner, kCardInk);
  const int bar = hasBar(c) ? kBar : 0;
  if (bar) {
    r.round(x, y, w, bar + kCorner, kCorner, rim);
    r.round(x, y + bar, w, kCorner, 0, kCardInk);
    const int room = w - 2 * CardLayout::kPad - 40;
    text(r, kSmallFont, x + 12, y + (bar - kSmallFont.h) / 2 + 1, c.agent, kBarWords, room);
    for (int i = 0; i < 3; ++i) r.disk(x + w - 16 - i * 11, y + bar / 2, 3, kBarWords);
  }
  r.ring(x, y, w, h, kCorner, 2, rim);
  const int room = w - 2 * CardLayout::kPad;
  const int tw = std::min(stringWidth(kMain, c.text), room);
  text(r, kMain, x + (w - tw) / 2, y + bar + pad(c), c.text, kWords, room);
  if (hasSmall(c)) {
    const int sw = std::min(stringWidth(kSmallFont, c.small), room);
    text(r, kSmallFont, x + (w - sw) / 2, y + bar + pad(c) + kMain.h - 2, c.small, c.over ? kOver : kSmallWords, room);
  }
  // A little gel hand at either side, gripping it.
  for (int side : {-1, 1}) {
    const int hx = side < 0 ? x : x + w, hy = y + bar + (h - bar) / 2;
    r.disk(hx, hy, CardLayout::kHand, kGel);
    r.alpha = 160;
    r.disk(hx - 4, hy - 5, 3, kShine);
    r.alpha = 255;
  }
  // The dots: one for each card there is, this one lit in its colour.
  if (c.count > 1) {
    const int gap = 14, x0 = s.width / 2 - (c.count - 1) * gap / 2, dy = b.y1 + down + CardLayout::kDotsRow / 2;
    for (int i = 0; i < c.count; ++i) r.disk(x0 + i * gap, dy, CardLayout::kDot + (i + 1 == c.index), i + 1 == c.index ? rim : kDotOff);
  }
}

}  // namespace render::gel
