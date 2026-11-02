#include "render/gel/low_sign.h"

#include <algorithm>
#include <cstring>

#include "render/font.h"
#include "render/gel/lane.h"
#include "render/gel/raster.h"
#include "render/maths.h"
#include "render/palette.h"
#include "render/screens.h"

namespace render::gel {
namespace {

constexpr int kPad = 16, kMarkWide = 18, kMarkGap = 10, kRim = 2, kRadius = 16, kCornerGap = 4;
// The rows between the lines, and from them to the card's rim, by the size;
// the fonts' cells have room of their own above and below.
constexpr int kLineStep = 27, kTallPad = 4;
constexpr int kHand = 10;  // the hands on its top corners, as a reaction's sign's
constexpr uint16_t kFill = rgb565(22, 20, 18), kGel = rgb565(89, 169, 224), kShine = rgb565(184, 233, 245);

// Where the card's top rests: its hands, a little above it, stay in the
// lane's rows.
int topOf(int height) { return height * kLaneTop / kHeight + kHand - 2; }

// A tick, a cross or three dots, 18 px wide, its left at x, its middle on cy.
void paintMark(Raster& r, LowSign::Mark mark, int x, int cy, uint16_t ink) {
  auto at = [&](int dx, int dy) { return Point{(x + dx) * 16, (cy + dy) * 16}; };
  switch (mark) {
    case LowSign::Mark::kTick: {
      const Point v[] = {at(2, 0), at(7, 5), at(16, -6)};
      r.stroke16(v, 3, 3 * 16, ink);
      break;
    }
    case LowSign::Mark::kCross: {
      const Point a[] = {at(3, -6), at(15, 6)}, b[] = {at(15, -6), at(3, 6)};
      r.stroke16(a, 2, 3 * 16, ink);
      r.stroke16(b, 2, 3 * 16, ink);
      break;
    }
    case LowSign::Mark::kDots:
      for (int i = 0; i < 3; ++i) r.disk16((x + 3 + 6 * i) * 16, cy * 16, 40, ink);
      break;
    case LowSign::Mark::kNone: break;
  }
}

}  // namespace

LowSignLayout lowSignLayout(int width, int height, int corner, const char* text, bool marked) {
  LowSignLayout l;
  char folded[LowSignLayout::kLineBytes];
  foldText(text, kLowSignMostGlyphs, folded, sizeof folded);
  const int frame = 2 * kPad + (marked ? kMarkWide + kMarkGap : 0);
  const int top = topOf(height);
  // The card's bottom row sets how wide it may be, past the panel's corners.
  auto room = [&](int tall) {
    const int edge = Surface::insetOf(width, height, corner, top, top + tall, kRadius) + kCornerGap;
    return width - 2 * edge - frame;
  };
  const int tallSign = kSign.h + 2 * kTallPad, tallSmall = kLarge.h + 2 * kTallPad,
            tallTwo = kLarge.h + kLineStep + 2 * kTallPad;
  const int glyphs = glyphCount(folded);
  int tall;
  if (glyphs * kSign.w <= room(tallSign)) {
    l.large = true, l.lines = 1, tall = tallSign;
    std::strcpy(l.line[0], folded);
  } else if (glyphs * kLarge.w <= room(tallSmall)) {
    l.lines = 1, tall = tallSmall;
    std::strcpy(l.line[0], folded);
  } else {
    tall = tallTwo;
    l.lines = wrapText(folded, room(tallTwo) / kLarge.w, 2, l.line[0], LowSignLayout::kLineBytes);
  }
  const Font& f = l.large ? kSign : kLarge;
  int widest = 0;
  for (int i = 0; i < l.lines; ++i) widest = std::max(widest, glyphCount(l.line[i]) * f.w);
  l.w = frame + widest, l.h = tall;
  l.x = (width - l.w) / 2, l.y = top;
  return l;
}

int lowSignDrop(const LowSign& sign, int height) {
  const int most = height - topOf(height);
  int drop = 0;
  if (sign.ms < uint32_t(kLowSignRiseMs)) drop = most - most * ease(int(sign.ms), kLowSignRiseMs) / 1024;
  if (sign.leftMs >= 0 && sign.leftMs < kLowSignFallMs)
    drop = std::max(drop, most - most * ease(sign.leftMs, kLowSignFallMs) / 1024);
  return drop;
}

uint32_t lowSignKey(const LowSign& sign) {
  uint32_t h = 2166136261u;
  auto add = [&](const void* data, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const uint8_t*>(data)[i]) * 16777619u;
  };
  if (sign.text) add(sign.text, std::strlen(sign.text));
  // The height doesn't matter here: any drop but 0 changes as it moves.
  const int numbers[] = {int(sign.mark), int(sign.ink), lowSignDrop(sign, kHeight * 16)};
  add(numbers, sizeof numbers);
  return h ? h : 1;
}

void paintLowSign(Surface& s, const LowSign& sign) {
  if (!s.pixels || !sign.shows()) return;
  const bool marked = sign.mark != LowSign::Mark::kNone;
  const LowSignLayout l = lowSignLayout(s.width, s.height, s.corner, sign.text, marked);
  const Font& f = l.large ? kSign : kLarge;
  Raster r{s};
  r.limit = s.height;
  const int y = l.y + lowSignDrop(sign, s.height);
  r.round(l.x, y, l.w, l.h, kRadius, sign.ink);
  r.round(l.x + kRim, y + kRim, l.w - 2 * kRim, l.h - 2 * kRim, kRadius - kRim, kFill);
  int x = l.x + kPad;
  const int textTop = y + kTallPad;
  const int linesTall = f.h + (l.lines - 1) * kLineStep;
  if (marked) {
    paintMark(r, sign.mark, x, textTop + linesTall / 2, sign.ink);
    x += kMarkWide + kMarkGap;
  }
  for (int i = 0; i < l.lines; ++i) text(r, f, x, textTop + i * kLineStep, l.line[i], sign.ink);
  // Boop's hands on its top corners, holding it up.
  for (int side : {-1, 1}) {
    const int hx = side < 0 ? l.x + kRadius / 2 : l.x + l.w - kRadius / 2;
    r.disk(hx, y + 2, kHand, kGel);
    r.alpha = 160;
    r.disk(hx - 3, y - 2, 3, kShine);
    r.alpha = 255;
  }
}

}  // namespace render::gel
