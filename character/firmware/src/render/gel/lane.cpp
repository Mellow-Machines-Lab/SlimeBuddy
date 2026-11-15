#include "render/gel/lane.h"
#include "render/gel/act.h"
#include "render/gel/card.h"
#include "render/gel/done.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "fonts_gel.h"
#include "render/gel/drawer.h"
#include "render/gel/engine.h"
#include "render/font.h"
#include "render/gel/layers.h"
#include "render/gel/painter.h"
#include "render/gel/pastimes.h"
#include "render/gel/profile.h"
#include "render/gel/raster.h"
#include "render/gel/stream.h"
#include "render/palette.h"

namespace render::gel {

namespace {

// The canvas's lane (render/screens.h) at the surface's size: the same
// rows, in proportion, and the same inks, at full strength.
struct Lane {
  Raster r;
  int w, h;
  int y(int canvasRow) const { return h * canvasRow / kHeight; }
};

uint16_t inkRgb565(int ink) { return paletteAt(inkAt(ink, kLevels)); }

// How far the body ducks under the needs-you card, at its whole.
constexpr int kAlertDuckPx = 12;
constexpr int kMargin = 18;  // from the panel's edges, as the canvas's 12 px
constexpr int kIcon = 18;    // the strip's marks, and the gap after them
// How far what's laid out by the panel's rounded corners keeps from them
// (Surface::inset), as their very edge may show dim.
constexpr int kCornerGap = 4;
constexpr int kBubblePad = 12;  // from the outline to the text, left and right
constexpr int kBubbleLine = 2;
constexpr int kBubbleRadius = 14;
// The bubble's box: from just under the rule, as tall as the sign's font
// and its outline, a pixel either side.
constexpr int kBubbleUnderRule = 4, kBubbleTall = 44;
// The drawer's grabber, in the rule's middle: its size, and the rule's gap
// either side of it.
constexpr int kGrabberW = 44, kGrabberH = 5, kGrabberGap = 10;
// The marks' numbers, and the gap after a mark and its number before the next.
constexpr int kMarkGap = 4, kMarkAfter = 14;

// The lane's rows on a surface: the rule (the strip's top), and the one
// row everything else takes, by its middle.
int ruleRow(int height) { return height * kStripTop / kHeight; }
int rowMiddle(int height) { return height * ((kStripTop + kHeight) / 2) / kHeight; }

// The bubble's rows, and how far in from either side it stands: clear of
// the panel's rounded corners, its own corners cut away like theirs.
struct BubbleBox {
  int top, bottom, margin;
  explicit BubbleBox(const Surface& s)
      : top(ruleRow(s.height) + kBubbleUnderRule),
        bottom(top + kBubbleTall),
        margin(std::max(kMargin, s.inset(top, bottom, kBubbleRadius) + kCornerGap)) {}
  int room(int width) const { return width - 2 * (margin + kBubblePad + kBubbleLine); }
};

// One glyph's 4-bit coverage, its cell's top left at (x, y): only the part
// of its cell inside the raster's bounds, the surface and above its bottom
// (a kept card draws its corners again through bounds a few pixels wide).
void glyph(Raster& r, const Font& f, int x, int y, int g, uint16_t ink) {
  const int rowBytes = (f.w + 1) / 2, opacity = r.alpha;
  const uint8_t* src = f.data + size_t(g) * rowBytes * f.h;
  const int bottom = r.limit ? r.limit : r.s.height * 4 / 5;
  const int x0 = std::max({x, r.bounds.x0, 0}), x1 = std::min({x + int(f.w), r.bounds.x1, r.s.width});
  const int y0 = std::max({y, r.bounds.y0, 0}), y1 = std::min({y + int(f.h), r.bounds.y1, bottom});
  if (x0 >= x1 || y0 >= y1) return;
  r.touch(x0, y0, x1, y1);
  for (int yy = y0 - y; yy < y1 - y; ++yy)
    for (int xx = x0 - x; xx < x1 - x; ++xx) {
      const int b = src[yy * rowBytes + xx / 2], coverage = xx & 1 ? b & 15 : b >> 4;
      if (!coverage) continue;
      r.alpha = opacity * coverage / 15;
      r.pixel(x + xx, y + yy, ink);
    }
  r.alpha = opacity;
}

}  // namespace

const Font& bubbleFont() { return kFontBubble; }

int text(Raster& r, const Font& f, int x, int y, const char* s, uint16_t ink, int room) {
  const int n = glyphCount(s);
  const bool fits = n * f.w <= room;
  for (int left = fits ? n : cutAt(s, room / f.w - 1), g; left > 0 && (g = nextGlyph(s)) >= 0; --left, x += f.w)
    glyph(r, f, x, y, g, ink);
  // Cut, it ends in one cell's "…", where the canvas's ".." takes two.
  if (!fits && f.count > kEllipsis) glyph(r, f, x, y, kEllipsis, ink), x += f.w;
  return x;
}

void text16(Raster& r, const Font& f, int x16, int y16, const char* s, uint16_t ink, bool behind) {
  // On the stack, not the board's scarce internal heap: a word forward, in
  // the widest font (the bubble's, 20 px).
  constexpr int kMostGlyphs = 20, kMostCells = kMostGlyphs * 20 + 1;
  if (!r.s.pixels || r.alpha <= 2 || f.w * kMostGlyphs >= kMostCells) return;
  int glyphs[kMostGlyphs], n = 0;
  for (int g; n < kMostGlyphs && (g = nextGlyph(s)) >= 0;) glyphs[n++] = g;
  const int x0 = x16 >> 4, y0 = y16 >> 4, fx = x16 & 15, fy = y16 & 15;
  const int cells = n * f.w, rows = f.h + (fy ? 1 : 0), rowBytes = (f.w + 1) / 2;
  // What it may draw on: the surface, the raster's bounds, and above its
  // bottom (as Raster::pixel: the lane's top unless a limit is set).
  const int bottom = r.limit ? r.limit : r.s.height * 4 / 5;
  const int top = std::max(0, r.bounds.y0), under = std::min(bottom, r.bounds.y1);
  const int left = std::max(0, r.bounds.x0), right = std::min(r.s.width, r.bounds.x1);
  // Columns u of the string's cells drawn, and the cell before each.
  const int uFrom = std::max(0, left - x0), uTo = std::min(cells + (fx ? 1 : 0), right - x0);
  if (uFrom >= uTo) return;
  // A row of the string's cells (0–15) at index u + 1, a zero before them;
  // whether it has any.
  uint8_t rowA[kMostCells + 2] = {}, rowB[kMostCells + 2] = {};
  uint8_t *here = rowA, *above = rowB;
  bool hereInk = false, aboveInk = false;
  auto decode = [&](int v, uint8_t* out) {
    if (v < 0 || v >= f.h) return false;
    bool any = false;
    for (int i = 0; i < n; ++i) {
      const uint8_t* src = f.data + size_t(glyphs[i]) * rowBytes * f.h + v * rowBytes;
      uint8_t* o = out + 1 + i * f.w;
      for (int xx = 0; xx < f.w; xx += 2) {
        const uint8_t b = src[xx >> 1];
        any |= b != 0;
        o[xx] = b >> 4;
        if (xx + 1 < f.w) o[xx + 1] = b & 15;
      }
    }
    return any;
  };
  // An opacity from a sample's sum (at most 15 × 256), as Raster::text's
  // and Raster::pixel's: the shape's opacity, then the layer's fade.
  const int opacity = r.fade >= 255 ? r.alpha : r.alpha * r.fade / 255;
  // Behind, over black: the ink at each sixtieth of the most a sum can be.
  uint16_t shade[61];
  if (behind)
    for (int k = 0; k <= 60; ++k) shade[k] = mix565(0, ink, opacity * k / 60);
  const int wx = 16 - fx, wy = 16 - fy;
  int bx0 = 1 << 30, by0 = 1 << 30, bx1 = -1, by1 = -1;
  for (int v = 0; v < rows; ++v) {
    const bool stale = aboveInk;  // the row about to be written over had ink
    std::swap(here, above);
    aboveInk = hereInk;
    if (stale) std::fill(here, here + cells + 2, 0);
    hereInk = decode(v, here);
    const int y = y0 + v;
    if (y < top || y >= under || !(hereInk || (fy && aboveInk))) continue;
    uint16_t* row = r.s.pixels + y * r.s.width + x0;
    int lo = 1 << 30, hi = -1;
    for (int u = uFrom; u < uTo; ++u) {
      // The cells at u and u − 1 (index u + 1 and u), this row and the one
      // above, weighted by how near.
      const int sum = fx ? (wx * here[u + 1] + fx * here[u]) * wy + (wx * above[u + 1] + fx * above[u]) * fy
                         : (wy * here[u + 1] + fy * above[u + 1]) * 16;
      if (!sum) continue;
      uint16_t& out = row[u];
      if (behind) {
        if (out) continue;
        out = shade[sum / 64];
      } else {
        const int a = opacity * sum / (15 * 256);
        if (a >= 253)
          out = ink;
        else if (a > 2)
          out = mix565(out, ink, a);
        else
          continue;
      }
      lo = std::min(lo, u), hi = u;
    }
    if (hi < 0) continue;
    bx0 = std::min(bx0, x0 + lo), bx1 = std::max(bx1, x0 + hi), by0 = std::min(by0, y), by1 = y;
  }
  if (bx1 >= 0) r.touch(bx0, by0, bx1 + 1, by1 + 1);
}

namespace {

// The line in its bubble, on the lane's one row under the rule: a box with
// round corners, the text centred in the sign's font, or the large one
// when it doesn't fit its width (on the AMOLED, the longest take, 22, fits
// the sign's), cut to fit, in warm white: amber is needs you's alone.
void bubble(Lane& l, const char* line) {
  const BubbleBox box(l.r.s);
  const int room = box.room(l.w);
  const Font& f = glyphCount(line) * kSign.w <= room ? kSign : kLarge;
  const int tw = std::min(glyphCount(line) * f.w, room);
  const int bw = tw + 2 * (kBubblePad + kBubbleLine), left = (l.w - bw) / 2;
  l.r.ring(left, box.top, bw, box.bottom - box.top, kBubbleRadius, kBubbleLine, rgb565(kGrabberRgb));
  text(l.r, f, (l.w - tw) / 2, (box.top + box.bottom) / 2 - f.h / 2, line, rgb565(kEyeRgb), room);
}

// The caption: the reaction's reason, the app's narration of why, in the
// large font and a light grey with no box, on the lane's row, so it reads
// as a note, not as something said.
void caption(Lane& l, const char* line) {
  const BubbleBox box(l.r.s);
  const int room = box.room(l.w), tw = std::min(glyphCount(line) * kLarge.w, room);
  text(l.r, kLarge, (l.w - tw) / 2, rowMiddle(l.h) - kLarge.h / 2, line, rgb565(kNarrationRgb), room);
}

// The strip's marks, each in a kIcon box with its middle at (x + 9, cy).
void dot(Raster& r, int x, int cy, uint16_t ink) { r.disk(x + 9, cy, 8, ink); }
// The working count's: a spinner of kSpinSteps spokes turning clockwise,
// the one on `spin` lit and the ones it passed fading behind it, as a
// computer shows it's busy.
void spinner(Raster& r, int x, int cy, int spin, uint16_t ink) {
  // Each spoke's way out from the middle, in sixteenths of a pixel per
  // pixel, clockwise from twelve o'clock.
  static constexpr Point kWay[kSpinSteps] = {{0, -16}, {11, -11}, {16, 0},  {11, 11},
                                             {0, 16},  {-11, 11}, {-16, 0}, {-11, -11}};
  const int cx16 = (x + 9) * 16 + 8, cy16 = cy * 16 + 8;
  for (int i = 0; i < kSpinSteps; ++i) {
    const int behind = (spin - i + kSpinSteps) % kSpinSteps;
    const Point spoke[] = {{cx16 + kWay[i].x * 4, cy16 + kWay[i].y * 4}, {cx16 + kWay[i].x * 8, cy16 + kWay[i].y * 8}};
    r.stroke16(spoke, 2, 40, mix565(0, ink, 255 - behind * 27));
  }
}
// The idle count's: a sleeper's z, and a smaller one rising after it.
void sleeper(Raster& r, int x, int cy, uint16_t ink) {
  r.zee((x + 1) * 16, (cy + 7) * 16, 12 * 16, ink);
  r.zee((x + 12) * 16, (cy - 2) * 16, 7 * 16, ink);
}
}  // namespace
void tick(Raster& r, int x, int cy, uint16_t ink) {
  r.line({x + 2, cy}, {x + 7, cy + 5}, 3, ink);
  r.line({x + 7, cy + 5}, {x + 16, cy - 6}, 3, ink);
}
void cross(Raster& r, int x, int cy, uint16_t ink) {
  r.line({x + 3, cy - 6}, {x + 15, cy + 6}, 3, ink);
  r.line({x + 15, cy - 6}, {x + 3, cy + 6}, 3, ink);
}
namespace {
void dots(Raster& r, int x, int cy, uint16_t ink) {
  for (int i = 0; i < 3; ++i) r.disk(x + 3 + 6 * i, cy + 5, 2, ink);
}
// A plug on its cord, pointing at nothing, 48 × 36 px, its left at x.
void plug(Raster& r, int x, int cy, uint16_t ink) {
  r.round(x, cy - 4, 16, 8, 4, ink);         // cord
  r.round(x + 14, cy - 18, 20, 36, 6, ink);  // body
  r.round(x + 34, cy - 12, 14, 6, 2, ink);   // prongs
  r.round(x + 34, cy + 6, 14, 6, 2, ink);
}

// Which of the counts the strip shows (counts()'s `shown`).
enum : unsigned { kShowBusy = 1, kShowHelpers = 2, kShowIdle = 4, kShowTodo = 8, kShowAll = 15 };

// The counts as marks, when a name takes the room: how many sessions
// work, by a spinner on `spin`, the helpers at work as "+N" after them,
// the agents idle, by a sleeper's z, and last how far down its to-do
// list the longest turn with one is ("3/7"), from x, each with the gap
// after it, those in `shown` alone. Drawn when r is given; either way, the
// x after them, so the names leave them the room they take.
int marks(Raster* r, const Strip& s, int x, int cy, int spin, unsigned shown) {
  const Font& f = kSign;
  const int ty = cy - f.h / 2;
  const uint16_t white = rgb565(kEyeRgb), grey = rgb565(kNarrationRgb);
  char n[24];
  auto number = [&](int at, uint16_t ink) { return r ? text(*r, f, at, ty, n, ink) : at + glyphCount(n) * f.w; };
  auto count = [&](int value, auto mark) {
    std::snprintf(n, sizeof(n), "%d", value);
    if (r) mark(x);
    return number(x + kIcon + kMarkGap, white) + kMarkAfter;
  };
  if (s.busy > 0 && shown & kShowBusy) x = count(s.busy, [&](int at) { spinner(*r, at, cy, spin, white); });
  if (s.helpers > 0 && shown & kShowHelpers) {
    std::snprintf(n, sizeof(n), "+%d", s.helpers);
    x = number(x, grey) + kMarkAfter;
  }
  if (s.idle > 0 && shown & kShowIdle) x = count(s.idle, [&](int at) { sleeper(*r, at, cy, grey); });
  if (s.todoTotal > 0 && shown & kShowTodo) {
    std::snprintf(n, sizeof(n), "%d/%d", s.todoDone, s.todoTotal);
    x = number(x, white) + kMarkAfter;
  }
  return x;
}

// The counts in words, when there's room: "2 working +4 helpers · 1 idle",
// the numbers in warm white and the words in grey, from x; `helpers` says
// how the helpers show: 2 with their word, 1 as "+4" alone, 0 not at all,
// as the room gets tighter. The to-do list isn't
// in it: it stands at the strip's right end. Drawn when r is given; either
// way, the x after them.
int words(Raster* r, const Strip& s, int x, int cy, int helpers) {
  const Font& f = kSign;
  const int ty = cy - f.h / 2;
  const uint16_t white = rgb565(kEyeRgb), grey = rgb565(kNarrationRgb);
  auto put = [&](const char* v, uint16_t ink) { x = r ? text(*r, f, x, ty, v, ink) : x + glyphCount(v) * f.w; };
  char n[16];
  if (s.busy > 0) {
    std::snprintf(n, sizeof(n), "%d", s.busy);
    put(n, white), put(" working", grey);
    if (s.helpers > 0 && helpers > 0) {
      std::snprintf(n, sizeof(n), " +%d", s.helpers);
      put(n, white);
      if (helpers > 1) put(s.helpers == 1 ? " helper" : " helpers", grey);
    }
  }
  if (s.idle > 0) {
    if (s.busy > 0) put(" \xC2\xB7 ", grey);
    std::snprintf(n, sizeof(n), "%d", s.idle);
    put(n, white), put(" idle", grey);
  }
  return x;
}

// The to-do list's "3/7", in warm white, its right end at `right`; the x
// it starts at, or `right` with none.
int todo(Raster* r, const Strip& s, int right, int cy) {
  if (s.todoTotal <= 0) return right;
  char n[16];
  std::snprintf(n, sizeof(n), "%d/%d", s.todoDone, s.todoTotal);
  const int x = right - glyphCount(n) * kSign.w;
  if (r) text(*r, kSign, x, cy - kSign.h / 2, n, rgb565(kEyeRgb));
  return x;
}

// The counts from x to `right`: in words if they fit, the to-do list at
// the right end, else as marks. Drawn when r is given; the x after them.
int counts(Raster* r, const Strip& s, int x, int right, int cy, int spin) {
  const int list = todo(nullptr, s, right, cy);
  const int room = list - (s.todoTotal > 0 ? kMarkAfter : 0) - x;
  for (int helpers : {2, 1, 0})
    if (words(nullptr, s, 0, cy, helpers) <= room) {
      todo(r, s, right, cy);
      return std::max(words(r, s, x, cy, helpers), s.todoTotal > 0 ? right : 0);
    }
  return marks(r, s, x, cy, spin, kShowAll);
}

}  // namespace

int paintCounts(Raster& r, const Strip& s, int x, int right, int cy, int spin) {
  return counts(&r, s, x, right, cy, spin);
}

void paintStatusMark(Raster& r, StatusMark mark, int x, int cy, int spin) {
  switch (mark) {
    case StatusMark::kWorking: spinner(r, x, cy, spin, kWorkingInk); break;
    case StatusMark::kIdle: sleeper(r, x, cy, kIdleInk); break;
    case StatusMark::kNeedsYou: dot(r, x, cy, rgb565(kAmberRgb)); break;
  }
}

void paintRule(Raster& r, int x0, int x1, int y, bool grabber) {
  const uint16_t rule = rgb565(kRuleRgb);
  if (!grabber) {
    r.round(x0, y, x1 - x0, 2, 1, rule);
    return;
  }
  const int cx = (x0 + x1) / 2, gap = kGrabberW / 2 + kGrabberGap;
  r.round(x0, y, cx - gap - x0, 2, 1, rule);
  r.round(cx + gap, y, x1 - cx - gap, 2, 1, rule);
  r.round(cx - kGrabberW / 2, y + 1 - kGrabberH / 2, kGrabberW, kGrabberH, kGrabberH / 2, rgb565(kGrabberRgb));
}

namespace {

// How far in from the panel's sides the strip's row stands: in from the
// sides, and clear of the panel's rounded corners on the row's rows.
int stripMargin(const Surface& panel, int cy) {
  return std::max(kMargin, panel.inset(cy - kSign.h / 2, cy + kSign.h / 2) + kCornerGap);
}

// The rule along the lane's top, the drawer's grabber in its middle while
// the drawer has rows to show: there whatever the lane's row holds.
void rule(Lane& l, const Strip& s) {
  const int y = ruleRow(l.h), margin = std::max(kMargin, l.r.s.inset(y, y + 2) + kCornerGap);
  paintRule(l.r, margin, l.w - margin, y, s.agent || s.busy > 0 || s.idle > 0);
}

// The status strip's row: who needs you, or whose turn the finish was,
// then how many sessions work; with no app, the plug and "no Mac" in the
// middle (render/screens.h's Strip).
void strip(Lane& l, const Strip& s, int spin) {
  if (!s.shows()) return;
  Raster& r = l.r;
  const Font& f = kSign;
  const int cy = rowMiddle(l.h), ty = cy - f.h / 2;
  const int margin = stripMargin(r.s, cy), right = l.w - margin, names = margin + 2 * kIcon;
  const uint16_t grey = rgb565(kNarrationRgb);
  if (s.noApp) {
    // No Mac: the plug, twice the strip's marks, and the words beside it.
    constexpr int kPlug = 48, kGap = 16;
    const char* words = "no Mac";
    const int w = kPlug + kGap + glyphCount(words) * f.w, x = (l.w - w) / 2;
    plug(r, x, cy, grey);
    text(r, f, x + kPlug + kGap, ty, words, grey);
    return;
  }
  // The room the marks in `shown` take after the names: the gap before
  // them, and up to the last one's end, so they stop at the margin.
  auto marksRoom = [&](unsigned shown) {
    const int w = marks(nullptr, s, 0, cy, spin, shown);
    return w ? kIcon + w - kMarkAfter : 0;
  };
  if (s.agent) {
    // Who's asking, which the needs-you card doesn't say: its thread (or
    // project) first, whole if it can be, then the agent before it, and
    // only then the working and idle counts, while there's room for them;
    // the helpers and the to-do list wait until nothing needs you. The
    // first of these that fits, else the thread cut to the room there is.
    // How many more wait stands apart at the right end, in a pill.
    const uint16_t amber = rgb565(kAmberRgb);
    // On the name's lower case's middle, a little under the cell's.
    dot(r, margin, cy + 3, amber);
    char more[12] = "";
    if (s.more > 0) std::snprintf(more, sizeof(more), "+%d", s.more);
    constexpr int kPillPad = 9, kPillGap = 12, kPillH = 30;
    const int pillW = more[0] ? glyphCount(more) * kLarge.w + 2 * kPillPad : 0;
    const int pillRoom = pillW ? pillW + kPillGap : 0;
    const char* what = s.name && *s.name ? s.name : s.project;
    const bool named = what && *what;
    char both[48];
    std::snprintf(both, sizeof(both), named ? "%s \xC2\xB7 %s" : "%s", s.agent, what);
    const char* const tries[] = {both, named ? what : s.agent};
    const unsigned chips[] = {kShowBusy | kShowIdle, kShowBusy, 0};
    const char* who = tries[1];
    unsigned shown = 0;
    bool whole = false;
    for (const char* name : tries)
      for (unsigned c : chips)
        if (!whole && glyphCount(name) * f.w <= right - names - pillRoom - marksRoom(c))
          who = name, shown = c, whole = true;
    const int x = text(r, f, names, ty, who, amber, right - names - pillRoom - marksRoom(shown));
    marks(&r, s, x + kIcon, cy, spin, shown);
    if (pillW) {
      const int px = right - pillW;
      r.ring(px, cy - kPillH / 2, pillW, kPillH, kPillH / 2, 2, amber);
      text(r, kLarge, px + kPillPad, cy - kLarge.h / 2, more, amber);
    }
    return;
  }
  if (s.doneAgent) {
    const uint16_t eye = rgb565(kEyeRgb);
    if (s.doneOutcome == Outcome::kSuccess)
      tick(r, margin, cy, rgb565(kSageRgb));
    else if (s.doneOutcome == Outcome::kFailure)
      cross(r, margin, cy, rgb565(kClayRgb));
    else
      dots(r, margin, cy, eye);
    // Whose turn it was, and its thread after a dot if it has one, cut to
    // what the counts leave; fewer counts if they'd leave less than the
    // agent's name.
    char who[48];
    if (s.doneThread && *s.doneThread)
      std::snprintf(who, sizeof(who), "%s \xC2\xB7 %s", s.doneAgent, s.doneThread);
    else
      std::snprintf(who, sizeof(who), "%s", s.doneAgent);
    const int least = (glyphCount(s.doneAgent) + 2) * f.w;
    const unsigned fewer[] = {kShowAll, kShowBusy | kShowIdle, kShowBusy, 0};
    unsigned shown = 0;
    for (unsigned c : fewer)
      if (right - names - marksRoom(c) >= least) {
        shown = c;
        break;
      }
    const int x = text(r, f, names, ty, who, eye, right - names - marksRoom(shown));
    marks(&r, s, x + kIcon, cy, spin, shown);
    return;
  }
  // Nothing named: the counts, in words when they fit.
  counts(&r, s, margin, right, cy, spin);
}

// The strip, drawn at full strength, blended once at its opacity (0–255):
// its rows hold nothing else, over black, so that is scaling each pixel it
// wrote, in place, with no buffer of its own; a hole (the working mark's
// inside, a mini's eyes) stays one, and where its shapes meet it blends
// once.
void fade(Surface& s, int from, int alpha) {
  if (alpha >= 253) return;
  const bool tracked = s.height <= Surface::kTrackedRows;
  for (int y = from; y < s.height; ++y) {
    uint16_t* row = s.pixels + y * s.width;
    const int x1 = tracked ? s.written[y].last : s.width;
    for (int x = tracked ? s.written[y].first : 0; x < x1; ++x)
      if (row[x]) row[x] = mix565(0, row[x], alpha);
  }
}

// What the lane shows, as a key that's the same exactly when its pixels
// are (FNV-1a over the line or the strip's fields; never 0).
}  // namespace

uint32_t laneKey(const char* line, const Strip& s, int spin, const char* caption, int rowAlpha) {
  uint32_t h = 2166136261u;
  auto add = [&](const void* data, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const uint8_t*>(data)[i]) * 16777619u;
  };
  auto str = [&](const char* v) { add(v ? v : "\x01", v ? std::strlen(v) + 1 : 1); };
  str(line);
  str(line ? nullptr : caption);
  const int grabber = s.agent || s.busy > 0 || s.idle > 0, shows = s.shows();
  add(&grabber, sizeof grabber), add(&shows, sizeof shows);
  if (!line && !caption) {
    str(s.agent), str(s.project), str(s.name), str(s.doneAgent), str(s.doneThread);
    const int numbers[] = {s.more,      int(s.doneOutcome), s.busy,  s.idle,    s.helpers,
                           s.todoDone,  s.todoTotal,        s.noApp, s.opacity, s.busy > 0 ? spin : 0,
                           rowAlpha};
    add(numbers, sizeof(numbers));
  }
  return h ? h : 1;
}

int bubbleRoom(const Surface& surface) { return BubbleBox(surface).room(surface.width); }

void paintLane(Surface& surface, const char* line, const Strip& s, int spin, const char* note, int rowAlpha) {
  if (!surface.pixels) return;
  Lane l{Raster{surface}, surface.width, surface.height};
  l.r.limit = surface.height;
  // One row under the rule: the bubble while Boop speaks, else the
  // caption, else the strip, which a low sign rising fades (rowAlpha).
  if (line || note || s.shows()) rule(l, s);
  if (line) {
    bubble(l, line);
  } else if (note) {
    caption(l, note);
  } else {
    strip(l, s, spin);
    fade(surface, ruleRow(l.h) + kGrabberH, rowAlpha);
    fade(surface, l.y(kLaneTop), (s.opacity * 255 + 500) / 1000);
  }
  surface.laneKey = laneKey(line, s, spin, note, rowAlpha);
  surface.laneKeyFrom = l.y(kLaneTop);
}

void paintFrame(Surface& surface, const Engine& e, bool mouthOpen, int dy, const char* line, const SignShow& sign,
                const Strip& s, const DoneShow& done, const rig::Channels& rig, const rig::Layers* layers, uint32_t t,
                const Stream* stream, uint32_t now, const PastimeShow* pastimes, const Drawer* drawer,
                const char* note, const WishShow* wish, const GrowthShow* growth, const CardShow* card) {
  rig::Anchors at;
  if (drawer && drawer->shows()) {
    // The lane's rows are the sheet's: erased, and drawn again after.
    const DrawerPose pose = drawerPose(surface, *drawer);
    paint(surface, e, mouthOpen, dy, 0, &at, rig, layers, t, nullptr, &pose);
    if (pose.squint) paintSquint(surface, at, pose, DrawerLayout(surface.width, surface.height, surface.corner).top(drawer->open));
    Measure measure(profile.lane);
    paintDrawer(surface, *drawer, s, at, spinStep(now));
    return;
  }
  // The sign takes the lane from the strip and the words; a newer
  // reaction's words take it back, as they take the sign down.
  const Stream::ActNow act = stream ? stream->actNow(now) : Stream::ActNow{};
  LowSign low = finishSign(done);
  // A finished turn's name outranks a work act's words.
  const bool actsSign = !low.shows();
  if (actsSign) low = actSign(act);
  // An act can run for long: what Boop says takes the lane from it meanwhile.
  if (low.shows() && (actsSign || low.leftMs >= 0) && (line || note)) low = LowSign{};
  // Something it remembers takes the lane from an act, as words do; its own
  // words have ended by then, their bubble going with them. A finished
  // turn's name outranks it.
  if (sign.low && sign.text && (actsSign || !low.shows())) {
    low = LowSign{};
    low.text = sign.text, low.ink = rgb565(kEyeRgb), low.ms = sign.ms, low.leftMs = sign.leftMs;
  }
  // A celebration's gold, a little after the sign it may come with: it
  // takes the lane from all of these, its rim glowing gold.
  const GoldShow gold = goldShow(sign);
  if (gold.sign.shows()) low = gold.sign;
  if (low.shows()) line = note = nullptr;
  // At rest it covers what's under it, so the strip's changes change nothing;
  // rising, the strip's row fades as it comes.
  const bool laneAtRest = low.shows() && lowSignDrop(low, surface.height) == 0;
  const int rowAlpha = low.shows() ? 255 - lowSignRisen(low, surface.height) : 255;
  // A lane that shows what this surface's already shows is kept, not
  // erased and drawn again (Surface::clear): the glow over it too.
  const int doneGlowPeak = doneGlow(done), glow = doneGlowPeak ? doneGlowPeak : gold.glow;
  const uint16_t glowInk = doneGlowPeak ? 0 : rgb565(kGoldRgb);
  const int spin = spinStep(now);
  uint32_t key = laneAtRest ? 1 : laneKey(line, s, spin, note, rowAlpha);
  if (low.shows()) key = ((key ^ lowSignKey(low)) * 16777619u) | 1;
  if (glow) key = ((key ^ uint32_t(glow << 8 | int(done.outcome) | (doneGlowPeak ? 0 : 0x80))) * 16777619u) | 1;
  // The card is Doing's: it gives way to a sign and to the low sign (a
  // finished turn's, a work act's, a celebration's gold), and comes back.
  const bool held = card && card->on && !sign.text && !low.shows();
  const int hop = held ? cardHop(*card) : signHop(sign);
  // The needs-you card stands in the top band: the body ducks under it.
  const int duck = (e.scenes.visible(Scenes::kAlertProp, e.timeMs()) * kAlertDuckPx).integer();
  paint(surface, e, mouthOpen, dy - hop, key, &at, rig, layers, t, pastimes, nullptr, duck);
  if (pastimes) {
    Measure measure(profile.props);
    paintPastimes(surface, *pastimes, at);
  }
  if (stream) {
    Measure measure(profile.stream);
    paintStreamBehind(surface, *stream, now);
    paintStreamFront(surface, *stream, at, now);
  }
  if (wish) paintWish(surface, *wish, at);
  if (sign.text && !sign.low) paintSign(surface, sign, at, hop);
  if (held) paintCard(surface, *card);
  if (growth) {
    // The top band holds one thing at a time: while LISTENING shows there,
    // the bar gives way to it (a heart flying still flies).
    const int listening = (e.scenes.visible(Scenes::kListeningProp, e.timeMs()) * 255).integer();
    paintGrowthBar(surface, *growth, at, nightcapFor(e.scenes.when), 255 - listening);
  }
  Measure measure(profile.lane);
  const bool lane = surface.laneKey != key;
  if (lane) {
    if (laneAtRest) {
      surface.laneKeyFrom = surface.height * kLaneTop / kHeight;
    } else {
      paintLane(surface, line, s, spin, note, rowAlpha);
    }
    paintLowSign(surface, low);
    surface.laneKey = key;
  }
  // A kept lane keeps its glow.
  const int glowTo = lane ? surface.height : surface.laneKeyFrom;
  if (doneGlowPeak)
    paintGlow(surface, done, 0, glowTo);
  else if (gold.glow)
    paintGlow(surface, gold.glow, glowInk, 0, glowTo);
}

}  // namespace render::gel
