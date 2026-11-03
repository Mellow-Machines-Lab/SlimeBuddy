#include "render/gel/act.h"

#include <algorithm>
#include <utility>

#include "render/gel/fixed.h"
#include "render/gel/frame.h"
#include "render/gel/raster.h"
#include "render/maths.h"
#include "render/palette.h"
#include "slime.h"

namespace render::gel {
namespace {

// The rocket's and the parachute's paints: each has its lit side and its
// shade, so it stands with the gel, which is lit from the upper left.
constexpr uint16_t kHull = rgb565(232, 238, 245), kHullLight = rgb565(255, 255, 255), kHullShade = rgb565(164, 178, 196),
                   kRed = rgb565(232, 70, 62), kRedLight = rgb565(255, 122, 104), kRedShade = rgb565(160, 34, 44),
                   kSteel = rgb565(92, 102, 118), kSteelDark = rgb565(52, 58, 70), kGlass = rgb565(64, 156, 226),
                   kGlassDeep = rgb565(26, 76, 130), kFlameOut = rgb565(255, 120, 24), kFlameMid = rgb565(255, 190, 44),
                   kFlameCore = rgb565(255, 244, 196), kChuteA = rgb565(246, 86, 80), kChuteAShade = rgb565(176, 48, 58),
                   kChuteB = rgb565(255, 208, 70), kChuteBShade = rgb565(204, 150, 40), kLine = rgb565(214, 222, 232),
                   kSoot = rgb565(18, 20, 26), kWhite = 0xFFFF;
constexpr uint16_t kSage = rgb565(kSageRgb), kClay = rgb565(kClayRgb), kEye = rgb565(kEyeRgb);

int X(const Surface& s, int p) { return s.width * p / 502; }
int Y(const Surface& s, int p) { return s.height * p / 410; }

// sin and cos of `t` 4096ths of a turn, Q12.
int sn(int t) { return sinTurn(t); }
int cs(int t) { return sinTurn(t + 1024); }
// The ink darkened toward the background's black, which blends without
// overlaps showing: `k` of 255.
uint16_t dim(uint16_t ink, int k) { return mix565(0, ink, std::clamp(k, 0, 255)); }

// Art in px of the 502, on the whole screen.
Frame artPlace(const Surface& s) { return {0, 0, 4096 * s.width / 502}; }
// A shape in art coordinates: at most 24 points, cut to `clip` if it's given.
struct Shape {
  const Frame& p;
  Raster& r;
  void fill(std::initializer_list<Point> pts, uint16_t ink, const Point* clip = nullptr, int clipN = 0) const {
    Point v[24];
    int n = 0;
    for (const Point& q : pts)
      if (n < 24) v[n++] = p.at(q.x, q.y);
    r.polygon16(v, n, ink, clip, clipN);
  }
  void disk(int x, int y, int rad, uint16_t ink) const {
    const Point c = p.at(x, y);
    r.disk16(c.x, c.y, p.size16(rad), ink);
  }
  void line(std::initializer_list<Point> pts, int width, uint16_t ink) const {
    Point v[12];
    int n = 0;
    for (const Point& q : pts)
      if (n < 12) v[n++] = p.at(q.x, q.y);
    r.stroke16(v, n, p.size16(width), ink);
  }
};

// The rocket, its seat at the origin: nose up, tail down.
constexpr Point kHullOutline[] = {{0, -172}, {16, -158}, {32, -134}, {46, -100}, {56, -62}, {62, -22}, {62, 32},
                                  {56, 72},  {44, 106},  {-44, 106}, {-56, 72},  {-62, 32}, {-62, -22}, {-56, -62},
                                  {-46, -100}, {-32, -134}, {-16, -158}};
constexpr int kHullPoints = int(sizeof kHullOutline / sizeof kHullOutline[0]);

// `soot` of 255: how burnt it is.
void paintRocket(Raster& r, const Frame& p, int soot = 0) {
  const Shape a{p, r};
  const int k = 255 - soot * 2 / 3;
  // The fins behind, each with a shaded lower edge.
  a.fill({{44, 26}, {66, 46}, {104, 112}, {108, 138}, {86, 128}, {44, 108}}, dim(kRedShade, k));
  a.fill({{44, 26}, {66, 46}, {104, 112}, {90, 112}, {44, 90}}, dim(kRed, k));
  a.fill({{-44, 26}, {-66, 46}, {-104, 112}, {-108, 138}, {-86, 128}, {-44, 108}}, dim(kRedShade, k));
  a.fill({{-44, 26}, {-66, 46}, {-104, 112}, {-90, 112}, {-44, 90}}, dim(kRedLight, k));
  // The nozzle.
  a.fill({{-26, 104}, {26, 104}, {36, 128}, {-36, 128}}, dim(kSteelDark, k));
  a.fill({{-26, 104}, {6, 104}, {6, 128}, {-36, 128}}, dim(kSteel, k));
  // The hull, its shade and its shine, each shape fitted to its outline,
  // which costs far less than cutting them to it.
  Point hull[kHullPoints];
  for (int i = 0; i < kHullPoints; ++i) hull[i] = p.at(kHullOutline[i].x, kHullOutline[i].y);
  r.polygon16(hull, kHullPoints, dim(kHull, k));
  a.fill({{24, -116}, {39, -116}, {46, -100}, {56, -62}, {62, -22}, {62, 32}, {56, 72}, {44, 106}, {18, 106}, {34, 40}, {36, -50}},
         dim(kHullShade, k));
  a.fill({{-40, -110}, {-34, -110}, {-44, -30}, {-46, 40}, {-54, 40}, {-56, -40}}, dim(kHullLight, k));
  // The red nose, lit and shaded the same way, and a band above the tail.
  a.fill({{0, -172}, {16, -158}, {32, -134}, {39, -116}, {0, -114}, {-39, -116}, {-32, -134}, {-16, -158}}, dim(kRed, k));
  a.fill({{10, -163}, {16, -158}, {32, -134}, {39, -116}, {24, -115}, {20, -140}}, dim(kRedShade, k));
  a.fill({{-20, -150}, {-12, -156}, {-24, -122}, {-33, -120}}, dim(kRedLight, k));
  a.fill({{-54, 76}, {54, 76}, {50, 90}, {-50, 90}}, dim(kRed, k));
  a.fill({{30, 76}, {54, 76}, {50, 90}, {28, 90}}, dim(kRedShade, k));
  // The window, up by the nose, in its steel ring.
  a.disk(0, -92, 15, dim(kSteel, k));
  a.disk(0, -92, 11, dim(kGlassDeep, k));
  a.disk(-2, -94, 8, dim(kGlass, k));
  a.disk(-4, -97, 3, dim(kWhite, k));
}

// The flame under the nozzle, `len` long, flickering by `ms`.
void paintFlame(Raster& r, const Frame& p, int len, int ms) {
  if (len < 4) return;
  const Shape a{p, r};
  const int flick = sn(ms * 37) * 9 / 4096 + sn(ms * 89 + 800) * 5 / 4096;
  const int sway = sn(ms * 23) * 5 / 4096;
  const int y0 = 126;
  a.fill({{-32, y0}, {32, y0}, {38, y0 + len * 3 / 10}, {24, y0 + len * 7 / 10}, {sway, y0 + len + flick},
          {-24, y0 + len * 7 / 10}, {-38, y0 + len * 3 / 10}},
         kFlameOut);
  a.fill({{-22, y0}, {22, y0}, {26, y0 + len * 3 / 10}, {15, y0 + len * 6 / 10}, {sway / 2, y0 + len * 8 / 10 - flick / 2},
          {-15, y0 + len * 6 / 10}, {-26, y0 + len * 3 / 10}},
         kFlameMid);
  a.fill({{-12, y0}, {12, y0}, {14, y0 + len * 2 / 10}, {0, y0 + len / 2 + flick / 2}, {-14, y0 + len * 2 / 10}}, kFlameCore);
}

// A little sky's stars, drifting down by `drift` px, twinkling by `ms`, in
// `k` of 255. The same few places every time.
void paintStars(Raster& r, const Surface& s, int drift, int ms, int k) {
  if (k < 4) return;
  static constexpr struct { int16_t x, y; uint8_t size, phase; } kStars[] = {
      {34, 66, 1, 0},   {78, 28, 2, 3},   {126, 96, 1, 5},  {172, 34, 1, 2},  {214, 118, 1, 6}, {62, 168, 2, 1},
      {112, 214, 1, 4}, {38, 262, 1, 7},  {318, 22, 2, 2},  {372, 84, 1, 6},  {428, 40, 2, 4},  {468, 124, 1, 1},
      {344, 158, 1, 3}, {408, 206, 2, 0}, {458, 262, 1, 5}, {296, 246, 1, 7}, {178, 286, 1, 3}, {348, 300, 1, 1},
  };
  const int span = Y(s, 328);
  for (const auto& st : kStars) {
    const int y = (Y(s, st.y) + drift * (2 + st.size) / 3) % span;
    const int tw = sn(ms * 2 / 3 + st.phase * 580);
    r.alpha = 255;
    r.disk16(X(s, st.x) * 16 + 8, y * 16 + 8, st.size == 2 ? 20 : 13, dim(kWhite, k * (170 + tw * 70 / 4096) / 255));
  }
}

// One billow of smoke: a darker ball with a lit cap, dim by `k` of 255, which
// fades to the black behind it with no overlaps showing.
void cloud(Raster& r, int x16, int y16, int rad16, int k) {
  if (k < 6 || rad16 < 16) return;
  r.alpha = 255;
  r.disk16(x16, y16, rad16, dim(rgb565(120, 128, 142), k));
  r.disk16(x16 - rad16 / 4, y16 - rad16 / 4, rad16 * 5 / 8, dim(rgb565(190, 196, 208), k));
}

// The billows along the ground from lighting up (`ms` since) on, spreading
// out either side and thinning once the rocket has gone.
void paintPadSmoke(Raster& r, const Surface& s, int ms) {
  if (ms >= 2300) return;  // thinned away
  static constexpr struct { int16_t dx; int16_t y; uint8_t rad; uint16_t delay; } kBillows[] = {
      {226, 306, 26, 500}, {-226, 304, 28, 520}, {176, 300, 32, 360}, {-180, 298, 34, 380}, {128, 304, 30, 200},
      {-124, 302, 32, 220}, {82, 306, 28, 90},   {-78, 306, 30, 110},  {40, 312, 24, 0},     {-38, 312, 24, 30},
  };
  const int cx = s.width * 8;
  for (const auto& b : kBillows) {
    const int age = ms - b.delay;
    if (age <= 0) continue;
    const int grow = easeOut12(rampIn(age, 0, 900));
    const int thin = 4096 - ease(ms, 1200, 2300);
    const int k = thin * 255 / 4096;
    const int rise = age * 6 / 1000 + (age > 1200 ? (age - 1200) * 10 / 1000 : 0);
    cloud(r, cx + X(s, b.dx) * 16 * (1024 + grow * 3 / 4) / 4096, (Y(s, b.y) - rise) * 16,
          X(s, b.rad) * 16 * (1024 + grow * 3 / 4) / 4096, k);
  }
}

// The way it went: a line of smoke up the middle of the sky, each puff laid
// as the rocket passed, smaller and fainter with height, drifting a little.
void paintTrail(Raster& r, const Surface& s, int age, int topY, int k) {
  if (k < 4) return;
  const int cx = s.width * 8, bottom = Y(s, 322);
  constexpr int kStep = 13;
  const uint16_t dark = dim(rgb565(120, 128, 142), k), light = dim(rgb565(206, 212, 222), k * 2 / 3);
  int i = 0;
  for (int y = bottom; y > -12; y -= kStep, ++i) {
    if (y < topY) break;
    const int up = 4096 - (y * 4096 / std::max(1, bottom));  // 0 at the foot, 4096 at the top
    const int wander = (sn(i * 260 + age / 7) * (2 + i / 3) + sn(i * 700 - age / 11) * 2) / 4096;
    const int rad = (13 * 16 - up * 10 * 16 / 4096) * (7 + (i * 5) % 4) / 10;
    r.alpha = 255;
    r.disk16(cx + wander * 16, y * 16, rad, dark);
    r.disk16(cx + wander * 16 - rad / 4, y * 16 - rad / 4, rad * 5 / 8, light);
  }
}

// The canopy, its rim's middle at the origin: gores of two colours round a
// dome, each with a shaded side, its rim scalloped; `wide` and `tall` in
// px. `sag` (0–4096) lets it collapse as it lands.
void paintCanopy(Raster& r, const Frame& p, int wide, int tall, int sag) {
  constexpr int kGores = 6, kRows = 8;
  const int w = wide - wide * sag / 8192, h = tall - tall * sag * 3 / (4 * 4096);
  auto meridian = [&](int g, int row) {
    // Gore line g of kGores round the front half, row from the rim (0) to
    // the top: x by the line's turn and the dome's width there, y up it.
    const int th = 2048 * g / kGores;  // left to right, half a turn
    const int up = 1024 * row / kRows;
    const int across = -cs(th);
    return Point{int(int64_t(w) * across / 4096 * cs(up) / 4096), -int(int64_t(h) * sn(up) / 4096)};
  };
  for (int g = 0; g < kGores; ++g) {
    Point v[2 * kRows + 2 + 1];
    int n = 0;
    for (int row = 0; row <= kRows; ++row) {
      const Point q = meridian(g, row);
      v[n++] = p.at(q.x, q.y);
    }
    for (int row = kRows - 1; row >= 0; --row) {
      const Point q = meridian(g + 1, row);
      v[n++] = p.at(q.x, q.y);
    }
    // The rim between the lines bows up.
    const Point left = meridian(g, 0), right = meridian(g + 1, 0);
    v[n++] = p.at((left.x + right.x) / 2, -tall / 7);
    const bool first = g % 2 == 0, lit = g < kGores / 2;
    r.polygon16(v, n, first ? (lit ? kChuteA : kChuteAShade) : (lit ? kChuteB : kChuteBShade));
  }
  // A shine high on the lit side.
  const Shape a{p, r};
  if (sag == 0) {
    r.alpha = 120;
    a.disk(-wide / 3, -tall * 3 / 5, wide / 16, kWhite);
    r.alpha = 255;
  }
}

// A soot mark on the body: a wide, uneven smear of see-through dark spots, not
// one round one, darkest where they overlap, with specks flung off its ends.
void smudge(Raster& r, int x16, int y16, int rad16, int alpha) {
  if (alpha <= 2) return;
  r.alpha = alpha / 2;
  constexpr struct { int dx, dy, rad; } kSpots[] = {
      {-62, 6, 34}, {-30, -10, 52}, {6, 6, 60}, {42, -6, 44}, {70, 8, 26},
      {-96, -22, 9}, {-104, 18, 6}, {94, -24, 8}, {108, 20, 5}, {30, 44, 7},
  };
  for (const auto& d : kSpots)
    r.disk16(x16 + rad16 * d.dx / 100, y16 + rad16 * d.dy / 100, rad16 * d.rad / 100, kSoot);
  r.alpha = 255;
}

// A puff of dust: plain and pale, thinning to the black behind it.
void dust(Raster& r, int x16, int y16, int rad16, int k) {
  if (k < 6) return;
  r.alpha = 255;
  r.disk16(x16, y16, rad16, dim(rgb565(132, 124, 114), k));
}

// A star for its head after a crash.
void spark(Raster& r, int x16, int y16, int rad16, int turn, uint16_t ink) {
  Point v[10];
  for (int i = 0; i < 10; ++i) {
    const int rr = i % 2 ? rad16 * 2 / 5 : rad16, a = turn + i * 4096 / 10;
    v[i] = {x16 + rr * cs(a) / 4096, y16 + rr * sn(a) / 4096};
  }
  r.polygon16(v, 10, ink);
}

// Stars going round its head after a crash, `after` since, gone by `endMs`: on
// an orbit `rx` by `ry` round `dy` below its crown, each `size` round.
void headStars(Raster& r, const Surface& s, const rig::Anchors& at, int after, int endMs, int rx, int dy, int ry, int size) {
  if (after >= endMs) return;
  r.alpha = 255 - rampIn(after, endMs - 500, endMs) * 255 / 4096;
  for (int i = 0; i < 4; ++i) {
    const int turn = after * 2 / 3 + i * 1024;
    spark(r, at.crown.x + X(s, rx) * 16 * cs(turn) / 4096, at.crown.y - Y(s, dy) * 16 + Y(s, ry) * 16 * sn(turn) / 4096,
          X(s, size) * 16, turn, rgb565(255, 216, 74));
  }
  r.alpha = 255;
}

// The tests. Everything stands on the track, on the floor, in px of the 502.
constexpr int kTrackY = 318, kWallX = 440, kSledFromX = 164, kSledToX = 292;
constexpr int kRideTests = 2750;  // its size, strapped on

// Where the sled is, its middle, this far in: it comes in from the left,
// then inches on, slower and slower, however long the tests take.
int sledXAt(int age) {
  if (age < kTestsSetMs) return lerp(-120, kSledFromX, easeOut12(rampIn(age, 0, kTestsSetMs)));
  const int run = age - kTestsSetMs;
  // The first minute takes it half way, each minute after half of what's left.
  const int span = kSledToX - kSledFromX;
  return kSledToX - int(uint32_t(span) * 60000u / uint32_t(60000 + run));
}
// The tremble while it holds its breath, in px.
int trembleOf(int age) { return age < kTestsSetMs ? 0 : sn(age * 73) * 3 / 4096 + sn(age * 131) * 1 / 4096; }

// The wall: a block in hazard stripes with a lit edge, `x` px from the left,
// on the floor.
void paintWall(Raster& r, const Surface& s, int x, int shake) {
  const Frame p = artPlace(s);
  const Shape a{p, r};
  const int x0 = x + shake, top = 6, bottom = kTrackY;
  a.fill({{x0, top}, {560, top}, {560, bottom}, {x0, bottom}}, rgb565(240, 190, 40));
  // Its stripes, slanting, cut to its face.
  r.bounds = {X(s, x0), Y(s, top), s.width, Y(s, bottom)};
  for (int y = top - 40; y < bottom + 80; y += 64)
    a.fill({{x0, y + 32}, {x0 + 80, y - 40}, {x0 + 80, y - 8}, {x0, y + 64}}, rgb565(28, 28, 30));
  r.bounds = Raster::kEverywhere;
  // Its lit edge, for a block with a front.
  a.fill({{x0, top}, {x0 + 4, top}, {x0 + 4, bottom}, {x0, bottom}}, rgb565(255, 232, 150));
}

// The floor, full width.
void paintFloor(const Shape& a) { a.fill({{0, kTrackY}, {502, kTrackY}, {502, kTrackY + 6}, {0, kTrackY + 6}}, rgb565(74, 84, 100)); }

// The floor and the start line's checks.
void paintTrack(Raster& r, const Surface& s, int alpha) {
  r.alpha = alpha;
  const Frame p = artPlace(s);
  const Shape a{p, r};
  paintFloor(a);
  for (int i = 0; i < 6; ++i) {
    const int y = kTrackY - 6 - i * 10;
    a.fill({{14, y}, {22, y}, {22, y + 10}, {14, y + 10}}, i % 2 ? rgb565(30, 30, 34) : rgb565(236, 240, 246));
    a.fill({{22, y}, {30, y}, {30, y + 10}, {22, y + 10}}, i % 2 ? rgb565(236, 240, 246) : rgb565(30, 30, 34));
  }
  r.alpha = 255;
}

// The sled: a red deck on two wheels, `tilt` of a turn, its deck's middle on
// `x`; the wheels `spin` round.
void paintSled(Raster& r, const Surface& s, int x, int spin) {
  const Frame p{X(s, x) * 16, Y(s, kTrackY) * 16, 4096 * s.width / 502};
  const Shape a{p, r};
  a.fill({{-96, -22}, {96, -22}, {104, -14}, {96, -8}, {-96, -8}, {-104, -14}}, kRed);
  a.fill({{-96, -13}, {100, -13}, {96, -8}, {-96, -8}}, kRedShade);
  a.fill({{-90, -22}, {90, -22}, {88, -19}, {-88, -19}}, kRedLight);
  for (int side : {-1, 1}) {
    a.disk(side * 62, -6, 9, kSteelDark);
    a.disk(side * 62, -6, 5, kSteel);
    const Point c = p.at(side * 62, -6), e = p.at(side * 62 + 4 * cs(spin) / 4096, -6 + 4 * sn(spin) / 4096);
    const Point spoke[2] = {c, e};
    r.stroke16(spoke, 2, 20, kWhite);
  }
}

// A crash-test target: a white disk quartered with black, `rad16` round.
void paintTarget(Raster& r, Point c, int rad16) {
  r.disk16(c.x, c.y, rad16 + 16, rgb565(24, 24, 28));
  r.disk16(c.x, c.y, rad16, kWhite);
  const Point q1[4] = {c, {c.x, c.y - rad16}, {c.x + rad16 * 7 / 10, c.y - rad16 * 7 / 10}, {c.x + rad16, c.y}};
  const Point q2[4] = {c, {c.x, c.y + rad16}, {c.x - rad16 * 7 / 10, c.y + rad16 * 7 / 10}, {c.x - rad16, c.y}};
  r.polygon16(q1, 4, rgb565(24, 24, 28));
  r.polygon16(q2, 4, rgb565(24, 24, 28));
}

// A dome worn on its head, the helmet's or the hard hat's: `wide` by `high`
// about its rim's middle, turned `tilt`.
struct Dome {
  int half, tall, mx, my, scale, wide = 0, high = 0, c = 4096, st = 0;
  static constexpr int kPts = 14;
  // Its rim dropNum/dropDen of the head's height below the brim line.
  Dome(const rig::Anchors& at, int dropNum, int dropDen, int widePct, int lift, int tilt) {
    const int brim = (at.brimL.y + at.brimR.y) / 2;
    half = (at.brimR.x - at.brimL.x) / 2;
    tall = std::max(32, brim - at.crown.y);
    mx = (at.brimL.x + at.brimR.x) / 2, my = brim + tall * dropNum / dropDen;
    scale = half * 4096 / (120 * 16);  // 1 at a full-size head's
    wide = half * widePct / 100, high = (my - at.crown.y) + lift * scale / 256;
    c = cs(tilt), st = sn(tilt);
  }
  Point turned(int x, int y) const {
    return {mx + int((int64_t(x) * c - int64_t(y) * st) / 4096), my + int((int64_t(x) * st + int64_t(y) * c) / 4096)};
  }
  Point rim(int a, int w, int h) const { return turned(-w * cs(a) / 4096, -h * sn(a) / 4096); }
  // The shell: an arc, closed through `bow` if it's given, and its shade, the
  // right of it, back up a smaller arc, a crescent widest at the rim.
  void paintShell(Raster& r, uint16_t ink, uint16_t shadeInk, int crescent, const Point* bow = nullptr) const {
    Point dome[kPts + 2];
    for (int i = 0; i <= kPts; ++i) dome[i] = rim(2048 * i / kPts, wide, high);
    if (bow) dome[kPts + 1] = *bow;
    r.polygon16(dome, kPts + 1 + (bow ? 1 : 0), ink);
    Point shade[kPts + 1];
    int n = 0;
    for (int i = 8; i <= kPts; ++i) shade[n++] = rim(2048 * i / kPts, wide, high);
    for (int i = kPts; i >= 9; --i) {
      const int in = 4096 - crescent * (i - 8) / (kPts - 8);
      shade[n++] = rim(2048 * i / kPts, int(int64_t(wide) * in / 4096), int(int64_t(high) * (4096 - (4096 - in) / 2) / 4096));
    }
    r.polygon16(shade, n, shadeInk);
  }
};

// The helmet on its head: a yellow dome over its top, lit from the left, its
// rim down near its brow, a target on its front. `tilt`
// turns it about the rim's middle.
void paintHelmet(Raster& r, const rig::Anchors& at, int tilt = 0) {
  const Dome d(at, 3, 5, 118, 20, tilt);
  const int wide = d.wide, high = d.high, tall = d.tall, scale = d.scale;
  const Point bow = d.turned(0, tall / 4);  // bowed with its brow
  d.paintShell(r, rgb565(246, 196, 32), rgb565(208, 150, 20), 1300, &bow);
  const Point band[3] = {d.turned(-wide, 0), bow, d.turned(wide, 0)};
  r.stroke16(band, 3, std::max(32, 8 * 16 * scale / 4096), rgb565(186, 130, 14));
  r.alpha = 170;
  const Point shine = d.rim(560, wide * 72 / 100, high * 76 / 100);
  r.disk16(shine.x, shine.y, std::max(16, 10 * 16 * scale / 4096), rgb565(255, 244, 196));
  r.alpha = 255;
  paintTarget(r, d.turned(0, -high * 56 / 100), std::max(32, 19 * 16 * scale / 4096));
}

// Its head's anchors while a hat comes down onto them, `age` into the act.
rig::Anchors droppedOn(const Surface& s, rig::Anchors at, int age) {
  if (age < 500) {
    const int up = Y(s, 200) * 16 * (4096 - easeOut12(rampIn(age, 0, 500))) / 4096;
    at.crown.y -= up, at.brimL.y -= up, at.brimR.y -= up;
  }
  return at;
}

ActPose testsPose(const Surface& s, const ActShow& act) {
  ActPose pose;
  const Point anchor = finishAnchor(s);
  const int floor = (Y(s, kTrackY - 22) - floorY(s)) * 16;  // on the sled's deck
  auto onSled = [&](int x, int scale) {
    pose.scale = scale;
    pose.dx16 = X(s, x) * 16 - anchor.x;
    pose.dy16 = floor;
  };
  const int age = act.ageMs;
  if (act.result == ActResult::kRunning) {
    const int x = sledXAt(age);
    onSled(x + trembleOf(age), kRideTests);
    if (age < 500) pose.scale = lerp(4096, kRideTests, ease(age, 0, 500)), pose.dx16 = pose.dx16 * ease(age, 0, 500) / 4096;
    // It breathes in, and holds it: a little bigger, and shaking.
    const int held = ease(age, kTestsSetMs, kTestsSetMs + 1400);
    pose.scale += 340 * held / 4096;
    pose.squash = -700 * held / 4096 + (sn(age * 91) * 90 / 4096) * held / 4096;
    return pose;
  }
  const int ms = act.resultMs;
  const int from = sledXAt(act.ageMs - ms);
  if (act.result == ActResult::kPass) {
    const int x = lerp(from, kWallX - 118, easeOut12(rampIn(ms, 0, kPassStopMs)));
    onSled(x, kRideTests + 340);
    // Lurches on as the sled stops, then lets it all out.
    if (ms < 500) pose.lean = sn(ms * 2048 / 500) * 1400 / 4096;
    const int out = ease(ms, 300, 1000);
    pose.scale = lerp(kRideTests + 340, kRideTests, out);
    pose.squash = lerp(-700, 300, out) + (ms > 300 && ms < 1300 ? sn((ms - 300) * 8) * 200 / 4096 : 0);
    return pose;
  }
  // Fail: the rush, the wall, flat and tall against it, then down it.
  if (ms < kSplatMs) {
    const int e = easeIn12(rampIn(ms, 0, kSplatMs));
    onSled(lerp(from, kWallX - 68, e), kRideTests + 340);
    pose.lean = 1400 * e / 4096;
    return pose;
  }
  const int after = ms - kSplatMs;
  const int down = ease(after, 300, kSlideMs);
  onSled(kWallX - 90, 2950);
  // Pressed thin and tall against it, then gathering into a heap at its foot.
  pose.squash = lerp(4600, -2600, down);
  pose.dx16 -= X(s, 50) * 16 * down / 4096;
  pose.dy16 += (Y(s, kTrackY - 2) - Y(s, kTrackY - 22)) * 16;
  pose.lean = -lerp(1600, 0, down);
  pose.dy16 -= Y(s, 16) * 16 * (4096 - down) / 4096;
  return pose;
}

void testsBehind(Raster& r, const Surface& s, const ActShow& act) {
  const int age = act.ageMs;
  const int in = easeOut12(rampIn(age, 0, kTestsSetMs));
  const bool failing = act.result == ActResult::kFail, passing = act.result == ActResult::kPass;
  const int hit = failing ? act.resultMs - kSplatMs : -1;
  int shake = 0;
  if (hit >= 0 && hit < 360) shake = sn(hit * 24) * 5 * (360 - hit) / (360 * 4096);
  paintWall(r, s, lerp(560, kWallX, in), shake);
  if (hit >= 0) {
    const Frame p = artPlace(s);
    const Shape a{p, r};
    const int k = rampIn(hit, 0, 120);
    a.line({{kWallX + shake, 120}, {kWallX + 16 + shake, 104}, {kWallX + 12 + shake, 88}, {kWallX + 30 + shake, 66}}, 3, rgb565(24, 24, 28));
    a.line({{kWallX + shake, 170}, {kWallX + 18 + shake, 182}, {kWallX + 28 + shake, 170}, {kWallX + 48 + shake, 188}}, 3, rgb565(24, 24, 28));
    a.line({{kWallX + shake, 220}, {kWallX + 12 + shake, 242}, {kWallX + 34 + shake, 250}}, 3, rgb565(24, 24, 28));
    // The gel it left on the wall: a splat, and drips running down.
    const int drip = ease(hit, 200, kSlideMs + 800);
    a.line({{kWallX + 6 + shake, 142}, {kWallX + 20 + shake, 156}}, 18 * k / 4096, rgb565(66, 150, 222));
    a.line({{kWallX + 24 + shake, 134}, {kWallX + 8 + shake, 160}}, 9 * k / 4096, rgb565(66, 150, 222));
    for (const auto& d : {std::pair<int, int>{4, 100}, std::pair<int, int>{14, 70}, std::pair<int, int>{22, 130}})
      a.line({{kWallX + d.first + shake, 150}, {kWallX + d.first + shake, 150 + d.second * drip / 4096}}, 3, rgb565(66, 150, 222));
  }
  paintTrack(r, s, in * 255 / 4096);
  // The sled: under it; at a fail it runs on to the wall and stops there.
  const int from = sledXAt(age - act.resultMs);
  int x = from;
  if (passing) {
    x = lerp(from, kWallX - 118, easeOut12(rampIn(act.resultMs, 0, kPassStopMs)));
  } else if (failing) {
    x = lerp(from, kWallX - 104, easeIn12(rampIn(act.resultMs, 0, kSplatMs)));
  }
  paintSled(r, s, x, x * 90 + age / 2);
}

// The bubble of its phew, with its word, from its mouth up to one side.
void phew(Raster& r, const Surface& s, const rig::Anchors& at, int ms) {
  const int k = rampIn(ms, 300, 600) * (4096 - rampIn(ms, 2300, 2800)) / 4096;
  if (k < 40) return;
  const int rise = rampIn(ms, 300, 2800);
  const int x = at.brimL.x - X(s, 8) * 16, y = at.crown.y - Y(s, 16 + 22 * rise / 4096) * 16;
  r.alpha = k * 240 / 4096;
  const uint16_t cloud = rgb565(226, 244, 255);
  r.disk16(at.brimL.x + X(s, 18) * 16, at.crown.y + Y(s, 8) * 16, X(s, 6) * 16, cloud);
  r.disk16(at.brimL.x + X(s, 4) * 16, at.crown.y - Y(s, 4 + 12 * rise / 4096) * 16, X(s, 10) * 16, cloud);
  const Point pill[2] = {{x - X(s, 22) * 16, y}, {x + X(s, 22) * 16, y}};
  r.stroke16(pill, 2, Y(s, 36) * 16, cloud);
  r.text("phew", x >> 4, y >> 4, rgb565(20, 44, 80), kLarge);
  r.alpha = 255;
}

// A gel hand, a thumb up, by its side.
void thumbsUp(Raster& r, const Surface& s, const rig::Anchors& at, int ms) {
  const int k = easeOut12(rampIn(ms, 900, 1250));
  if (!k) return;
  const uint16_t gel = rgb565(74, 158, 226), deep = rgb565(38, 110, 186), shine = rgb565(184, 233, 245);
  const int size = 4096 * s.width / 502 * k / 4096;
  const Frame p{at.hem[0].x - X(s, 30) * 16, std::min(at.brimL.y + Y(s, 74) * 16, Y(s, 250) * 16) + Y(s, 36) * 16 * (4096 - k) / 4096, size};
  const Shape a{p, r};
  a.line({{-6, -2}, {-3, -28}}, 16, gel);
  a.line({{-8, 12}, {8, 12}}, 32, gel);
  a.line({{-10, 25}, {10, 25}}, 8, deep);
  a.line({{8, 2}, {22, 2}}, 2, deep);
  a.line({{8, 13}, {22, 13}}, 2, deep);
  a.disk(-6, -31, 3, shine);
  a.disk(-18, 3, 3, shine);
}

void testsFront(Raster& r, const Surface& s, const ActShow& act, const rig::Anchors& at) {
  if (!act.full) {
    paintHelmet(r, at);
    return;
  }
  const int age = act.ageMs;
  const bool failing = act.result == ActResult::kFail;
  // The helmet comes down as it starts; knocked askew at the wall.
  int tilt = 0;
  if (failing) tilt = -lerp(0, 170, ease(act.resultMs, kSplatMs, kSplatMs + 400));
  paintHelmet(r, act.result == ActResult::kRunning ? droppedOn(s, at, age) : at, tilt);
  // A target on each of its sides.
    if (!failing || act.resultMs < kSplatMs) {
    // And the belt strapping it down, low across its front.
    const int yy = (at.eyeL.y + at.eyeR.y) / 2 + Y(s, 22) * 16;
    const int half = std::abs(at.eyeR.x - at.eyeL.x);
    const int cx = (at.eyeL.x + at.eyeR.x) / 2;
    paintTarget(r, {cx - half * 80 / 100, yy}, X(s, 9) * 16);
    paintTarget(r, {cx + half * 80 / 100, yy}, X(s, 9) * 16);
    r.alpha = 225;
    r.stroke16(at.hem, rig::Anchors::kRingPoints, X(s, 9) * 16, rgb565(40, 42, 50));
    r.alpha = 255;
  }
  if (act.result == ActResult::kRunning && age >= kTestsSetMs) {
    // Cheeks full, a bead of sweat running, and the shakes in the air.
    const int held = ease(age, kTestsSetMs, kTestsSetMs + 1400);
    const int half = std::abs(at.eyeR.x - at.eyeL.x), cx = (at.eyeL.x + at.eyeR.x) / 2, eye = (at.eyeL.y + at.eyeR.y) / 2;
    r.alpha = held * 150 / 4096;
    r.disk16(cx - half * 58 / 100, eye + Y(s, 15) * 16, X(s, 11) * 16, rgb565(255, 128, 150));
    r.disk16(cx + half * 58 / 100, eye + Y(s, 15) * 16, X(s, 11) * 16, rgb565(255, 128, 150));
    r.alpha = 255;
    const int beat = (age - kTestsSetMs) % 2200, run = rampIn(beat, 0, 1500);
    const int bx = at.brimR.x - X(s, 16) * 16, by = at.brimR.y + Y(s, 6) * 16 + Y(s, 42) * 16 * easeIn12(run) / 4096;
    r.alpha = 255 - rampIn(beat, 1300, 1700) * 255 / 4096;
    const Point drop[5] = {{bx, by - Y(s, 12) * 16}, {bx + 5 * 16, by - 2 * 16}, {bx + 3 * 16, by + 5 * 16}, {bx - 3 * 16, by + 5 * 16}, {bx - 5 * 16, by - 2 * 16}};
    r.polygon16(drop, 5, rgb565(170, 228, 255));
    r.alpha = held * 200 / 4096;
    for (int side : {-1, 1}) {
      const int ox = cx + side * (half + X(s, 32 + (sn(age * 19) * 2 / 4096)) * 16);
      for (int i = 0; i < 2; ++i) {
        const int y = eye - Y(s, 6 - i * 20) * 16;
        const Point arc[3] = {{ox + side * i * 5 * 16, y - 8 * 16}, {ox + side * (i * 5 + 5) * 16, y}, {ox + side * i * 5 * 16, y + 8 * 16}};
        r.stroke16(arc, 3, 40, rgb565(143, 211, 255));
      }
    }
    r.alpha = 255;
  }
  if (act.result == ActResult::kPass) {
    phew(r, s, at, act.resultMs);
    thumbsUp(r, s, at, act.resultMs);
  }
  if (failing && act.resultMs >= kSplatMs) {
    const int after = act.resultMs - kSplatMs;
    if (after < 200) {
      const int k = 4096 - rampIn(after, 0, 200);
      spark(r, X(s, kWallX) * 16, at.eyeL.y, X(s, 74) * 16 * k / 4096, 300, rgb565(255, 214, 80));
      spark(r, X(s, kWallX) * 16, at.eyeL.y, X(s, 40) * 16 * k / 4096, 700, rgb565(255, 248, 214));
    }
    headStars(r, s, at, after, 3200, 58, 18, 11, 11);
  }
}

// The build. The house stands on the floor to the left, its courses going up
// with time; Boop stands to the right, the hammer in a gel hand.
constexpr int kBuildBodyX = 330, kHouseX = 110, kHouseW = 138, kHouseWallTop = 212;
constexpr int kBuildSize = 3200;
constexpr int kCourses = 7, kCourseH = (kTrackY - kHouseWallTop) / kCourses;

// How built the house is this far in, 0–4096: courses of wall up to 3072, then
// the door, the windows and the roof.
int houseAt(int age) {
  const int p = rampIn(age, kBuildSetMs, kHouseMs);
  return easeOut12(p);
}

// The house, built to `built`; `done` raises its flag, 0–4096. `fall` (0–4096)
// shakes it apart: each course and the roof slide, tip and sink to rubble.
void paintHouse(Raster& r, const Surface& s, int built, int done, int fall) {
  const Frame at{X(s, kHouseX) * 16, Y(s, kTrackY) * 16, 4096 * s.width / 502};
  const Shape a{at, r};
  const uint16_t brick = rgb565(198, 116, 78), brickLit = rgb565(224, 148, 104), brickDeep = rgb565(138, 74, 52),
                 mortar = rgb565(92, 56, 44);
  const int hw = kHouseW / 2;
  const int courses = std::min(kCourses, built * (kCourses + 1) / 3072);
  for (int i = 0; i < courses; ++i) {
    // Each course of bricks, half a brick along from the last. As it falls
    // the higher ones go further and drop to the floor.
    const int y0 = -(i + 1) * kCourseH, h = kCourseH;
    const int dir = i % 2 ? 1 : -1;
    const int fx = dir * (10 + i * 6) * fall / 4096, fy = (-y0 - h) * easeIn12(fall) / 4096;
    // Once down, the courses break into the rubble below.
    r.alpha = 255 - rampIn(fall, 2600, 3800) * 255 / 4096;
    if (r.alpha < 3) continue;
    const Frame c = Frame{at.x16 + X(s, fx) * 16, at.y16 + Y(s, fy) * 16, at.scale}.turnedBy(dir * 120 * fall / 4096);
    const Shape b{c, r};
    b.fill({{-hw, y0}, {hw, y0}, {hw, y0 + h}, {-hw, y0 + h}}, brick);
    b.fill({{-hw, y0}, {hw, y0}, {hw, y0 + 3}, {-hw, y0 + 3}}, brickLit);
    b.fill({{hw - 22, y0}, {hw, y0}, {hw, y0 + h}, {hw - 22, y0 + h}}, brickDeep);
    b.fill({{-hw, y0 + h - 1}, {hw, y0 + h - 1}, {hw, y0 + h}, {-hw, y0 + h}}, mortar);
    for (int x = -hw + (i % 2 ? 22 : 44); x < hw; x += 44) b.fill({{x, y0}, {x + 1, y0}, {x + 1, y0 + h}, {x, y0 + h}}, mortar);
  }
  r.alpha = 255;
  const int finish = built > 3072 ? (built - 3072) * 4096 / 1024 : 0;
  if (fall < 1200 && finish > 0) {
    // The door and the windows, lit, once the walls are up.
    const int wall = -kCourses * kCourseH;
    r.alpha = std::min(4096, finish * 3) * 255 / 4096 * (1200 - fall) / 1200;
    a.fill({{-13, -46}, {13, -46}, {13, 0}, {-13, 0}}, rgb565(84, 54, 38));
    a.disk(7, -22, 2, rgb565(240, 200, 90));
    for (int side : {-1, 1}) {
      a.fill({{side * 44 - 13, wall + 22}, {side * 44 + 13, wall + 22}, {side * 44 + 13, wall + 48}, {side * 44 - 13, wall + 48}}, rgb565(255, 226, 130));
      a.fill({{side * 44 - 1, wall + 22}, {side * 44 + 1, wall + 22}, {side * 44 + 1, wall + 48}, {side * 44 - 1, wall + 48}}, mortar);
      a.fill({{side * 44 - 13, wall + 34}, {side * 44 + 13, wall + 34}, {side * 44 + 13, wall + 36}, {side * 44 - 13, wall + 36}}, mortar);
    }
    r.alpha = 255;
  }
  if (finish >= 1024) {
    // The roof comes down onto the walls, and at a fall slides off them.
    const int wall = -kCourses * kCourseH;
    const int come = rampIn(finish, 1024, 3200);
    const int fx = 30 * fall / 4096, fy = (-wall - 16) * easeIn12(fall) / 4096;
    const Frame c = Frame{at.x16 + X(s, fx) * 16, at.y16 + Y(s, fy - 40 * (4096 - come) / 4096) * 16, at.scale}.turnedBy(-260 * fall / 4096);
    const Shape b{c, r};
    r.alpha = rampIn(come, 0, 800) * 255 / 4096;
    b.fill({{-hw - 14, wall}, {0, wall - 58}, {hw + 14, wall}}, rgb565(216, 66, 58));
    b.fill({{0, wall - 58}, {hw + 14, wall}, {16, wall}}, rgb565(160, 38, 44));
    b.fill({{-hw - 14, wall - 4}, {hw + 14, wall - 4}, {hw + 14, wall}, {-hw - 14, wall}}, rgb565(118, 28, 36));
    r.alpha = 255;
    if (done > 0) {
      const int wallY = wall - 58, up = 34 * easeOut12(done) / 4096;
      b.line({{0, wallY}, {0, wallY - up}}, 2, kLine);
      const int w = 28 * done / 4096, flap = sn(done * 3 + 300) * 3 / 4096;
      b.fill({{1, wallY - up}, {1 + w, wallY - up + 8 + flap}, {1, wallY - up + 16}}, rgb565(92, 222, 122));
    }
  }
  if (fall > 200) {
    // Dust rising from it, and rubble lying where it fell.
    // Rubble where it fell: a heap, and loose bricks on and round it.
    r.alpha = rampIn(fall, 1400, 3000) * 255 / 4096;
    a.fill({{-hw - 14, 0}, {-hw + 8, -22}, {-24, -30}, {14, -38}, {hw - 10, -20}, {hw + 18, 0}}, brickDeep);
    a.fill({{-hw + 4, 0}, {-34, -12}, {18, -14}, {hw - 6, 0}}, mortar);
    static constexpr struct { int16_t x, y, turn; } kBricks[] = {
        {-70, -6, 300},  {-46, -24, -200}, {-14, -30, 120}, {20, -32, 500},  {44, -14, -340},
        {72, -5, 160},   {-24, -12, 640},  {14, -10, -120}, {-88, -3, -80},  {92, -4, 400},
    };
    for (const auto& b : kBricks) {
      const Frame q = Frame{at.x16 + X(s, b.x) * 16, at.y16 + Y(s, b.y) * 16, at.scale}.turnedBy(b.turn);
      const Shape c{q, r};
      c.fill({{-12, -6}, {12, -6}, {12, 6}, {-12, 6}}, brick);
      c.fill({{-12, -6}, {12, -6}, {12, -3}, {-12, -3}}, brickLit);
      c.fill({{6, -3}, {12, -3}, {12, 6}, {6, 6}}, brickDeep);
    }
    r.alpha = 255;
  }
}

// The hard hat: a yellow dome with a ridge over the crown and a brim all round.
void paintHardHat(Raster& r, const rig::Anchors& at, int tilt = 0) {
  const Dome d(at, 1, 4, 92, 26, tilt);
  const int half = d.half, tall = d.tall, wide = d.wide, high = d.high, scale = d.scale;
  auto turned = [&](int x, int y) { return d.turned(x, y); };
  const uint16_t yellow = rgb565(250, 200, 34), deep = rgb565(206, 148, 18), lit = rgb565(255, 228, 110), edge = rgb565(176, 120, 12);
  d.paintShell(r, yellow, deep, 1200);
  // The ridge front to back, as a raised band over its top.
  const int rw = half / 7;
  const Point ridge[6] = {turned(-rw, 0), turned(-rw * 3 / 4, -high * 98 / 100), turned(0, -high * 112 / 100), turned(rw * 3 / 4, -high * 98 / 100), turned(rw, 0), turned(0, tall / 20)};
  r.polygon16(ridge, 6, lit);
  const Point ridgeSide[4] = {turned(rw / 6, -high * 110 / 100), turned(rw * 3 / 4, -high * 98 / 100), turned(rw, 0), turned(rw / 4, 0)};
  r.polygon16(ridgeSide, 4, yellow);
  // The brim, wide, with an edge under it.
  const int bw = half * 128 / 100;
  const Point band[2] = {turned(-bw, 0), turned(bw, 0)};
  r.stroke16(band, 2, std::max(40, 15 * 16 * scale / 4096), edge);
  const Point band2[2] = {turned(-bw, -scale * 16 * 2 / 4096), turned(bw, -scale * 16 * 2 / 4096)};
  r.stroke16(band2, 2, std::max(32, 10 * 16 * scale / 4096), yellow);
  r.alpha = 170;
  const Point shine = d.rim(620, wide * 66 / 100, high * 62 / 100);
  r.disk16(shine.x, shine.y, std::max(16, 9 * 16 * scale / 4096), rgb565(255, 248, 210));
  r.alpha = 255;
}

// Where the hammer's swing is now: its angle from up (0) over toward the house
// (4096 down onto it), and how long since it last struck, -1 for none yet.
int swingOf(int age, int& sinceHit) {
  const int run = age - kBuildSetMs;
  if (run < 0) return 0;
  const int beat = run % kSwingMs;
  constexpr int kRaise = 460, kStrike = 120;  // up slowly, down fast, then it rests
  sinceHit = run >= kRaise + kStrike ? (beat >= kRaise + kStrike ? beat - kRaise - kStrike : beat + kSwingMs - kRaise - kStrike) : -1;
  if (beat < kRaise) return 4096 - ease(beat, 0, kRaise);
  if (beat < kRaise + kStrike) return easeIn12(rampIn(beat, kRaise, kRaise + kStrike));
  return 4096;
}

// The hammer in its gel hand: `swing` 0 raised, 4096 down on the house's top;
// its head's place into `head`.
void paintHammer(Raster& r, const Surface& s, const Point& hand, int swing, int restTurn, int downTurn, Point* head = nullptr) {
  const int turn = lerp(restTurn, downTurn, swing);
  const Frame p = Frame{hand.x, hand.y, 4096 * s.width / 502}.turnedBy(turn);
  const Shape a{p, r};
  a.line({{0, 0}, {0, -78}}, 6, rgb565(150, 100, 58));
  a.line({{-1, -2}, {-1, -76}}, 2, rgb565(196, 144, 92));
  a.fill({{-24, -94}, {20, -94}, {26, -86}, {26, -72}, {-24, -72}}, kSteel);
  a.fill({{-24, -94}, {20, -94}, {22, -90}, {-24, -90}}, rgb565(176, 186, 200));
  a.fill({{-24, -78}, {26, -78}, {26, -72}, {-24, -72}}, kSteelDark);
  if (head) *head = p.at(-22, -82);
  const uint16_t gel = rgb565(74, 158, 226);
  a.disk(0, 0, 15, gel);
  a.disk(-4, -5, 4, rgb565(184, 233, 245));
}

ActPose buildPose(const Surface& s, const ActShow& act) {
  ActPose pose;
  const Point anchor = finishAnchor(s);
  const int floor = (Y(s, kTrackY - 2) - floorY(s)) * 16;
  const int in = ease(act.ageMs, 0, kBuildSetMs);
  pose.scale = lerp(4096, kBuildSize, in);
  pose.dx16 = (X(s, kBuildBodyX) * 16 - anchor.x) * in / 4096;
  pose.dy16 = floor * in / 4096;
  if (act.result == ActResult::kRunning) {
    int since = -1;
    const int swing = swingOf(act.ageMs, since);
    pose.lean = -swing * 700 / 4096;
    if (since >= 0 && since < 140) pose.squash = -sn(since * 2048 / 140) * 700 / 4096;
    return pose;
  }
  const int ms = act.resultMs;
  if (act.result == ActResult::kPass) {
    // Looks at it, leaning a little, chest out.
    pose.lean = -ease(ms, 0, 500) * 500 / 4096;
    pose.squash = 500 * ease(ms, 200, 900) / 4096;
    return pose;
  }
  // It flinches as it comes down, and sags.
  if (ms < 400) pose.lean = sn(ms * 2048 / 400) * 1000 / 4096;
  pose.squash = -lerp(0, 900, ease(ms, 200, 1600));
  return pose;
}

void buildBehind(Raster& r, const Surface& s, const ActShow& act) {
  {
    const Frame p = artPlace(s);
    const Shape a{p, r};
    r.alpha = rampIn(act.ageMs, 0, kBuildSetMs) * 255 / 4096;
    paintFloor(a);
    r.alpha = 255;
  }
  const int ageAtEnd = act.result == ActResult::kRunning ? act.ageMs : act.ageMs - act.resultMs;
  int built = houseAt(ageAtEnd);
  int done = 0, fall = 0;
  if (act.result == ActResult::kPass) {
    // Whatever's left goes up at once, and the flag.
    built = lerp(built, 4096, easeOut12(rampIn(act.resultMs, 0, 700)));
    done = rampIn(act.resultMs, 600, 1200);
  } else if (act.result == ActResult::kFail) {
    fall = easeOut12(rampIn(act.resultMs, 250, 1300));
  }
  // Something on the floor from the start, even at a quick fail.
  built = std::max(built, 900);
  paintHouse(r, s, built, done, fall);
}

void buildFront(Raster& r, const Surface& s, const ActShow& act, const rig::Anchors& at) {
  if (!act.full) {
    paintHardHat(r, at);
    return;
  }
  const bool failing = act.result == ActResult::kFail;
  const int tilt = failing ? -lerp(0, 240, ease(act.resultMs, 100, 700)) : 0;
  paintHardHat(r, act.result == ActResult::kRunning ? droppedOn(s, at, act.ageMs) : at, tilt);
  // The hand at its left side, the hammer in it.
  const int half = std::abs(at.eyeR.x - at.eyeL.x);
  const Point hand{at.eyeL.x - half * 62 / 100, std::min(at.eyeL.y + Y(s, 30) * 16, Y(s, kTrackY - 34) * 16)};
  // Raised, on a low house's first course, on the walls' top; held up proud,
  // and hanging down.
  constexpr int kUpTurn = 300, kDownLow = -1000, kDownTurn = -500, kProudTurn = -110, kDroppedTurn = -1040;
  if (act.result == ActResult::kRunning) {
    if (act.ageMs < kBuildSetMs) return;
    int since = -1;
    const int swing = swingOf(act.ageMs, since);
    Point head;
    // Its stroke ends a little lower as the house is still low.
    const int built = houseAt(act.ageMs);
    const int down = lerp(kDownLow, kDownTurn, std::min(4096, built * 4096 / 3072));
    paintHammer(r, s, hand, swing, kUpTurn, down, &head);
    if (since >= 0 && since < 230) {
      const int k = 4096 - rampIn(since, 0, 230);
      spark(r, head.x - 8 * 16, head.y + 6 * 16, X(s, 40) * 16 * k / 4096, since, rgb565(255, 164, 40));
      spark(r, head.x - 8 * 16, head.y + 6 * 16, X(s, 24) * 16 * k / 4096, since + 400, rgb565(255, 232, 110));
      for (int i = 0; i < 6; ++i) {
        const int a = 1700 + i * 340 + (act.ageMs / kSwingMs * 137) % 200;
        const int reach = X(s, 18 + i * 3) * 16 + X(s, 44) * 16 * (4096 - k) / 4096;
        r.alpha = k * 255 / 4096;
        r.disk16(head.x - 8 * 16 + reach * cs(a) / 4096, head.y + 6 * 16 + reach * sn(a) / 4096 + since * since / 90, 22, rgb565(255, 224, 120));
      }
      r.alpha = 255;
    }
    return;
  }
  paintHammer(r, s, hand, 0, failing ? kDroppedTurn : kProudTurn, 0);
}

// The rocket's place at the opening's `age`; `away` once it's gone.
constexpr int kRocketY = 180;  // its seat, at rest on the pad
Frame rocketAt(const Surface& s, int age) {
  const int size = s.width * 4096 / 502;
  Frame p{s.width * 8, Y(s, kRocketY) * 16, size};
  if (age < kPushBoardMs) {
    p.y16 += Y(s, 330) * 16 * (4096 - easeOut12(rampIn(age, 0, kPushBoardMs))) / 4096;
  } else if (age < kPushLightMs) {
    const int rumble = rampIn(age, kPushBoardMs, kPushLightMs);
    p.x16 += sn(age * 61) * 2 * 16 / 4096 * rumble / 4096;
    p.y16 += sn(age * 47) * 16 / 4096 * rumble / 4096;
  } else {
    const int up = age - kPushLightMs;
    const int span = kPushGoneMs - kPushLightMs;
    p.y16 -= Y(s, 620) * 16 * easeIn12(rampIn(up, 0, span)) / 4096;
    p.x16 += sn(age * 61) * 16 / 4096;
  }
  return p;
}

// Across the sky in orbit, this far into a pass: nearly side on, small.
constexpr int kOrbitScale = 1250;
Frame orbitAt(const Surface& s, int ms) {
  const int p = rampIn(ms, 0, kOrbitMs);
  const int x = lerp(-70, 572, p), y = 112 - sn(p / 2) * 44 / 4096;
  return Frame{X(s, x) * 16, Y(s, y) * 16, kOrbitScale * s.width / 502}.turnedBy(1024 + (cs(p / 2) * 180 / 4096));
}
// The pass this far into the act, or -1.
int orbitMs(int age) {
  if (age < kOrbitFirstMs) return -1;
  const int ms = (age - kOrbitFirstMs) % kOrbitEveryMs;
  return ms < kOrbitMs ? ms : -1;
}

// Where it falls, on its parachute: how far above its landing, in px, swaying.
int chuteFall(int ms) { return 470 * (4096 - easeOut12(rampIn(ms, 0, kChuteLandMs))) / 4096; }
int chuteSway(int ms) {
  const int left = 4096 - rampIn(ms, 0, kChuteLandMs);
  return sn(ms * 3 / 2) * left / 4096;
}

// The crash: where the rocket stuck, tail in the air, nose in the ground.
Frame crashedRocket(const Surface& s, int ms) {
  const int size = s.width * 4096 * 3 / (502 * 4);
  const int p = rampIn(ms, 0, kCrashHitMs);
  const int settle = ms > kCrashHitMs ? sn((ms - kCrashHitMs) * 5) * std::max(0, 500 - (ms - kCrashHitMs)) / 500 * 3 / 4096 : 0;
  return Frame{X(s, lerp(560, 412, easeIn12(p))) * 16, Y(s, lerp(-220, 248, easeIn12(p))) * 16, size}.turnedBy(2048 - 300 + settle * 6);
}

}  // namespace

ActPose actPose(const Surface& s, const ActShow& act) {
  ActPose pose;
  if (act.kind == ActKind::kTests) return testsPose(s, act);
  if (act.kind == ActKind::kBuild) return buildPose(s, act);
  if (act.result == ActResult::kPass) {
    const int ms = act.resultMs;
    if (ms < kChuteLandMs) {
      pose.scale = 2900;
      pose.dy16 = -Y(s, chuteFall(ms)) * 16;
      pose.dx16 = X(s, chuteSway(ms) * 26 / 4096) * 16;
      pose.lean = -chuteSway(ms) * 280 / 4096;
      return pose;
    }
    // Down: a thump, then up to its own size, chest out.
    const int after = ms - kChuteLandMs;
    pose.scale = lerp(2900, 4096, ease(after, 120, 900));
    if (after < 500) pose.squash = -sn(after * 2048 / 500) * 1500 / 4096;
    return pose;
  }
  if (act.result == ActResult::kFail) {
    const int ms = act.resultMs;
    if (ms < kCrashHitMs) {
      const int p = easeIn12(rampIn(ms, 0, kCrashHitMs));
      pose.scale = 3300;
      pose.dy16 = -Y(s, 470) * 16 * (4096 - p) / 4096;
      pose.dx16 = -X(s, 40) * 16 * (4096 - p) / 4096;
      pose.squash = 1800;
      return pose;
    }
    const int after = ms - kCrashHitMs;
    // Flattened, wide, and slowly gathering itself.
    pose.dx16 = -X(s, 56) * 16;
    pose.dy16 = -Y(s, 5) * 16;
    pose.scale = lerp(3300, 3700, ease(after, 900, 2200));
    pose.squash = lerp(-3600, -900, ease(after, 500, 2200));
    pose.lean = sn(after * 2) * 120 / 4096 * std::max(0, 2000 - after) / 2000;
    return pose;
  }
  // Running: on the rocket, then gone; a pass in orbit, small.
  const int age = act.ageMs;
  const int orbit = orbitMs(age);
  if (age >= kPushGoneMs && orbit < 0) {
    pose.away = true;
    return pose;
  }
  // Riding: smaller, in front of the hull.
  constexpr int kRide = 2500;
  const Point anchor = finishAnchor(s);
  auto seat = [&](const Frame& p, int scale) {
    // The body's middle, some 115 px above its floor at full size, on the seat.
    const int middle = anchor.y - Y(s, 104) * 16 * scale / 4096;
    pose.scale = scale;
    pose.dx16 = p.x16 - anchor.x, pose.dy16 = p.y16 - middle;
  };
  if (orbit >= 0) {
    seat(orbitAt(s, orbit), 760);
    return pose;
  }
  const Frame p = rocketAt(s, age);
  if (age < kPushBoardMs) {
    const int e = ease(age, 80, kPushBoardMs - 100);
    seat(Frame{s.width * 8, Y(s, kRocketY) * 16, 4096}, lerp(4096, kRide, e));
    pose.dy16 = pose.dy16 * e / 4096;
    pose.squash = -sn(ease(age, 0, 620) / 2) * 520 / 4096;
    return pose;
  }
  seat(p, kRide);
  if (age >= kPushLightMs) pose.squash = 1400 * rampIn(age, kPushLightMs, kPushLightMs + 500) / 4096;
  else pose.squash = -600 * rampIn(age, kPushBoardMs, kPushLightMs) / 4096;
  return pose;
}

void paintActBehind(Surface& s, const ActShow& act) {
  if (!s.pixels) return;
  Raster r{s};
  if (act.kind == ActKind::kTests) return testsBehind(r, s, act);
  if (act.kind == ActKind::kBuild) return buildBehind(r, s, act);
  if (act.result == ActResult::kPass) {
    const int ms = act.resultMs;
    paintStars(r, s, 0, ms + act.ageMs, 255 - ease(ms, kChuteLandMs - 400, kChuteLandMs + 600) * 255 / 4096);
    return;
  }
  if (act.result == ActResult::kFail) {
    const int ms = act.resultMs;
    paintStars(r, s, 0, ms + act.ageMs, 255 - ease(ms, kCrashHitMs - 200, kCrashHitMs + 80) * 255 / 4096);
    paintRocket(r, crashedRocket(s, ms), std::min(255, ms * 255 / kCrashHitMs));
    return;
  }
  const int age = act.ageMs;
  const int orbit = orbitMs(age);
  // The sky comes as it rises, and stays.
  const int climbed = easeIn12(rampIn(age, kPushLightMs, kPushGoneMs));
  const int sky = ease(age, kPushBoardMs, kPushLightMs + 600) * 255 / 4096;
  paintStars(r, s, Y(s, 500) * climbed / 4096 + std::max(0, age - kPushGoneMs) / 300, age, sky);
  if (age >= kPushLightMs) paintPadSmoke(r, s, age - kPushBoardMs);
  else if (age >= kPushBoardMs + 200) paintPadSmoke(r, s, age - kPushBoardMs - 200);
  if (age >= kPushLightMs) {
    // The trail stands where it rose; it thins but doesn't go.
    const Frame p = rocketAt(s, age);
    const int k = lerp(255, 120, ease(age, kPushGoneMs, kPushGoneMs + 6000));
    paintTrail(r, s, age, (p.y16 >> 4) + Y(s, 130), k);
  }
  if (orbit >= 0) {
    const Frame p = orbitAt(s, orbit);
    paintFlame(r, p, 44, age);
    paintRocket(r, p);
  } else if (age < kPushGoneMs) {
    const Frame p = rocketAt(s, age);
    int len = 0;
    if (age >= kPushBoardMs + 200) len = lerp(10, 44, rampIn(age, kPushBoardMs + 200, kPushLightMs));
    if (age >= kPushLightMs) len = lerp(44, 150, rampIn(age, kPushLightMs, kPushLightMs + 600));
    paintFlame(r, p, len, age);
    paintRocket(r, p);
  }
}

void paintActFront(Surface& s, const ActShow& act, const rig::Anchors& at) {
  if (!act.shows() || !s.pixels) return;
  Raster r{s};
  if (act.kind == ActKind::kTests) return testsFront(r, s, act, at);
  if (act.kind == ActKind::kBuild) return buildFront(r, s, act, at);
  if (!act.full) return;
  if (act.result == ActResult::kPass) {
    const int ms = act.resultMs;
    if (ms >= kChuteLandMs + 900) return;
    // Lines from the rim to its sides, the canopy above; as it lands it
    // lets go, sagging away to one side and fading. A little dust either
    // side at the thump.
    const int after = std::max(0, ms - kChuteLandMs);
    const int go = easeOut12(rampIn(after, 0, 900)), k = 255 - rampIn(after, 450, 900) * 255 / 4096;
    const int size = s.width * 4096 / 502;
    const int sway = chuteSway(ms);
    const Point crown = at.crown;
    const Frame chute = Frame{crown.x + X(s, 8) * sway * 16 / 4096 + X(s, 200) * 16 * go / 4096,
                              crown.y - Y(s, 34) * 16 + Y(s, 150) * 16 * go / 4096, size}.turnedBy(sway * 90 / 4096 + go * 700 / 4096);
    const int rim = 138, tall = 92;
    if (after == 0) {
      for (const Point& a : {at.brimL, at.brimR}) {
        const int side = a.x < crown.x ? -1 : 1;
        const Point rimAt = chute.at(side * rim, 0), mid = chute.at(side * rim / 3, 0);
        const Point v1[2] = {rimAt, a}, v2[2] = {mid, a};
        r.stroke16(v1, 2, 24, kLine);
        r.stroke16(v2, 2, 20, kLine);
      }
    }
    r.fade = k;
    paintCanopy(r, chute, rim, tall, go * 3 / 4);
    r.fade = 255;
    if (after > 0 && after < 800) {
      const int p = rampIn(after, 0, 800), spread = easeOut12(p);
      for (int side : {-1, 1})
        for (int i = 0; i < 2; ++i)
          dust(r, s.width * 8 + side * X(s, 112 + i * 28 + 60 * spread / 4096) * 16,
               (floorY(s) - 8) * 16 - Y(s, 10 + i * 4) * 16 * p / 4096, X(s, 15 - i * 3 + 14 * p / 4096) * 16,
               (4096 - p) * 200 / 4096);
    }
    return;
  }
  if (act.result == ActResult::kFail) {
    const int ms = act.resultMs, after = ms - kCrashHitMs;
    if (after < 0) return;
    // Soot on it, stars round its head, smoke going up from the wreck, the
    // ground heaped at its foot, and a burst and clods as it hits.
    const int cx = (at.eyeL.x + at.eyeR.x) / 2, eyeY = (at.eyeL.y + at.eyeR.y) / 2;
    const int w = std::abs(at.eyeR.x - at.eyeL.x), up = std::max(16, eyeY - at.crown.y);
    const int soot = rampIn(after, 0, 80) * (4096 - rampIn(act.leftMs >= 0 ? 700 - act.leftMs : 0, 0, 700)) / 4096 * 140 / 4096;
    smudge(r, cx + w * 30 / 100, eyeY - up * 64 / 100, X(s, 34) * 16, soot);
    smudge(r, cx - w * 66 / 100, eyeY - up * 10 / 100, X(s, 22) * 16, soot);
    headStars(r, s, at, after, 2800, 72, 8, 13, 12);
    const Frame wreck = crashedRocket(s, ms);
    const Point tail = wreck.at(0, 120);
    for (int i = 0; i < 6; ++i) {
      const int life = (after + i * 300) % 1800, p = life * 4096 / 1800;
      const int kk = (4096 - p) * 215 / 4096 * rampIn(after, 0, 300) / 4096;
      cloud(r, tail.x + (sn(life + i * 700) * 7 * 16 / 4096) - X(s, 12) * 16 * p / 4096, tail.y - Y(s, 150) * 16 * p / 4096,
            X(s, 9 + 15 * p / 4096) * 16, kk);
    }
    const Frame ground = artPlace(s);
    const Shape g{ground, r};
    g.fill({{338, 334}, {352, 314}, {376, 300}, {404, 306}, {432, 296}, {462, 308}, {486, 320}, {500, 334}}, rgb565(74, 54, 42));
    g.fill({{358, 334}, {372, 318}, {398, 314}, {428, 308}, {456, 318}, {476, 334}}, rgb565(50, 36, 30));
    if (after < 300) {
      // The burst at its foot, shrinking fast.
      const int k = 4096 - rampIn(after, 0, 300);
      const int bx = X(s, 404) * 16, by = Y(s, 306) * 16;
      r.alpha = 255;
      spark(r, bx, by, X(s, 96) * 16 * k / 4096, 300, rgb565(255, 140, 40));
      spark(r, bx, by, X(s, 66) * 16 * k / 4096, 700, rgb565(255, 214, 80));
      spark(r, bx, by, X(s, 34) * 16 * k / 4096, 300, rgb565(255, 248, 214));
    }
    if (after < 700) {
      static constexpr struct { int16_t vx, vy; uint8_t rad; } kClods[] = {
          {-90, -120, 6}, {-50, -160, 4}, {-10, -180, 5}, {30, -150, 7}, {60, -110, 4}, {-120, -80, 4}, {90, -90, 5}};
      for (const auto& c : kClods) {
        const int fly = after * 16;
        const int x = X(s, 404) * 16 + c.vx * fly / 500, y = Y(s, 300) * 16 + c.vy * fly / 500 + after * after * 16 / 800;
        r.alpha = 255 - rampIn(after, 500, 700) * 255 / 4096;
        r.disk16(x, y, c.rad * 16, rgb565(112, 82, 60));
      }
      r.alpha = 255;
    }
  }
}

int actFace(const ActShow& act, int mood) {
  if (!actTakesOver(act)) return -1;
  // A cross or low mood shows through: it acts it out with its own face.
  const bool cross = mood == slime::kAnnoyed || mood == slime::kIrritated || mood == slime::kGrumpy ||
                     mood == slime::kAngry || mood == slime::kSad || mood == slime::kWounded;
  if (act.kind == ActKind::kBuild) {
    if (act.result == ActResult::kFail) return act.resultMs < 900 ? slime::kStunned : cross ? -1 : slime::kDisappointed;
    if (act.result == ActResult::kPass) return cross ? -1 : act.resultMs < 700 ? slime::kHappy : slime::kProud;
    return cross ? -1 : slime::kDetermined;
  }
  if (act.kind == ActKind::kTests) {
    if (act.result == ActResult::kFail) return act.resultMs < kSplatMs + 1400 ? slime::kStunned : cross ? -1 : slime::kWounded;
    if (act.result == ActResult::kPass) return cross ? -1 : act.resultMs < 2500 ? slime::kRelieved : slime::kProud;
    return cross ? -1 : act.ageMs < kTestsSetMs ? slime::kDetermined : slime::kUneasy;
  }
  if (act.result == ActResult::kFail)
    return act.resultMs < kCrashHitMs + 1500 ? slime::kStunned : cross ? -1 : slime::kDisappointed;
  if (cross) return -1;
  if (act.result == ActResult::kPass) return act.resultMs < kChuteLandMs ? slime::kHappy : slime::kProud;
  return act.ageMs < kPushLightMs ? slime::kExcited : slime::kHappy;
}

LowSign actSign(const ActShow& act) {
  LowSign sign;
  if (!act.shows()) return sign;
  sign.text = act.text;
  if (act.result == ActResult::kRunning) {
    sign.mark = LowSign::Mark::kDots;
    sign.ink = kEye;
    sign.ms = uint32_t(act.ageMs);
  } else {
    sign.mark = act.result == ActResult::kPass ? LowSign::Mark::kTick : LowSign::Mark::kCross;
    sign.ink = act.result == ActResult::kPass ? kSage : kClay;
    sign.ms = uint32_t(kLowSignRiseMs);  // over the running one: the words change in its place
    sign.leftMs = act.leftMs;
  }
  return sign;
}

}  // namespace render::gel
