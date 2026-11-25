#include "render/gel/invite.h"

#include <algorithm>
#include <cstring>

#include "render/font.h"
#include "render/gel/finish.h"
#include "render/gel/fixed.h"
#include "render/gel/frame.h"
#include "render/gel/lane.h"
#include "render/gel/raster.h"
#include "render/palette.h"

namespace render::gel {

namespace {

using Phase = InviteShow::Phase;

// The props' inks (render/gel/layers.cpp): a dark rim, the gel of a hand and
// its shine; the glow's warm light, sparks' pale gold, a heart's pink.
constexpr uint16_t kRim = rgb565(27, 55, 78), kGel = rgb565(89, 169, 224), kShine = rgb565(184, 233, 245);
constexpr uint16_t kCream = rgb565(246, 240, 226), kGlow = rgb565(255, 226, 150), kWhite = rgb565(255, 255, 255);
constexpr uint16_t kSpark = rgb565(250, 230, 160), kHeart = rgb565(226, 78, 104);

// A hump from a to b, 0 at either end and 4096 at its middle.
int bump(int t, int a, int b) { return t <= a || t >= b ? 0 : sinTurn(int(int64_t(t - a) * 2048 / (b - a))); }
int of(int v, int q12) { return int(int64_t(v) * q12 / 4096); }

// How far up to the glass it is (Q12): in with the pose, back as the
// payoff ends or as it shrugs; a late one comes as it turns round.
int closeness(const InviteShow& v) {
  const int ms = int(v.ms);
  switch (v.phase) {
    case Phase::kPose: return ease(ms, 0, kInviteInMs);
    case Phase::kPayoff: return 4096 - ease(ms, int(InviteShow::kPayoffMs) - kInviteOutMs, int(InviteShow::kPayoffMs));
    case Phase::kShrug: return 4096 - ease(ms, 0, int(InviteShow::kShrugMs) * 3 / 4);
    case Phase::kLate:
      return ms < int(InviteShow::kTurnMs)
                 ? ease(ms, 0, int(InviteShow::kTurnMs))
                 : 4096 - ease(ms, int(InviteShow::kLateMs) - kInviteOutMs, int(InviteShow::kLateMs));
    default: return 0;
  }
}

// How far its face is turned away, over its shoulder (Q12): the back
// pose's, turning round as it sighs or shrugs; a late one's as it turns
// round to you.
int turned(const InviteShow& v) {
  const int ms = int(v.ms);
  if (v.phase == Phase::kLate)
    return ms < int(InviteShow::kTurnMs) ? 2900 - of(2900, ease(ms, 0, int(InviteShow::kTurnMs))) : 0;
  if (v.pose != InvitePose::kBack) return 0;
  switch (v.phase) {
    case Phase::kPose: return ease(ms, 0, kInviteInMs);
    case Phase::kShrug: return 4096 - ease(ms, 200, int(InviteShow::kShrugMs));
    case Phase::kPayoff: return 4096 - ease(ms, 300, 1100);
    default: return 0;
  }
}

// How much of the palm shows (Q12): up with the pose, held through the
// high five, down after it or at a miss.
int palmShown(const InviteShow& v) {
  if (v.pose != InvitePose::kPalm) return 0;
  const int ms = int(v.ms), age = payoffAge(v);
  switch (v.phase) {
    case Phase::kPose: return ease(ms, 80, kInviteInMs + 80);
    case Phase::kShrug: return 4096 - ease(ms, 0, 350);
    case Phase::kPayoff:
    case Phase::kLate: return age < 0 ? 0 : age < 120 ? ease(age, 0, 120) : 4096 - ease(age, 700, 1000);
    default: return 0;
  }
}

// How much of the glow and its word show (Q12): in just after the pose,
// out at a miss; a touch flashes it away (the payoff's ring).
int glowShown(const InviteShow& v) {
  const int ms = int(v.ms);
  if (v.phase == Phase::kPose) return ease(ms, 150, kInviteInMs + 150);
  if (v.phase == Phase::kShrug) return 4096 - ease(ms, 0, 250);
  if (v.phase == Phase::kPayoff) return 4096 - ease(ms, 0, 180);
  return 0;
}

// The body's scale now (Q12).
int scaleOf(const InviteShow& v) { return 4096 + of(kInviteScale[std::min(int(v.pose), 3)] - 4096, closeness(v)); }

// A circle of `n` points about (cx, cy), px, `radius` px.
void circle(Raster& r, int cx, int cy, int radius, int width16, uint16_t ink) {
  Point ring[25];
  for (int i = 0; i <= 24; ++i) {
    const int a = i * 4096 / 24;
    ring[i] = {cx * 16 + 8 + int(int64_t(radius) * 16 * sinTurn(a + 1024) / 4096),
               cy * 16 + 8 + int(int64_t(radius) * 16 * sinTurn(a) / 4096)};
  }
  r.stroke16(ring, 25, width16, ink);
}

// The glow: soft light breathing about the spot, brightest at its middle,
// and now and then a ripple going out from it.
void glow(Raster& r, Point at, int radius, int shown, int ms) {
  const int breath = 2900 + of(1196, (sinTurn(ms * 4096 / kGlowBreathMs) + 4096) / 2);
  const int a = of(shown, breath);
  // Many faint disks, so the light falls off smoothly, warm gold.
  for (int i = 9; i >= 0; --i) {
    r.alpha = of(26 - i * 2, a);
    r.disk(at.x, at.y, radius * (12 + i * 2) / 16, kGlow);
  }
  r.alpha = of(90, a);
  r.disk(at.x, at.y, radius * 9 / 16, kWhite);
  const int age = ms % kRippleMs;
  if (age < 1000) {
    r.alpha = of(of(220, 4096 - age * 4096 / 1000), shown);
    circle(r, at.x, at.y, radius * (18 + age * 22 / 1000) / 16, 3 * 16, kGlow);
  }
  r.alpha = 255;
}

// An open palm, flat on the glass, facing you: a dark rim, the gel, its
// heel pressed paler, and four fingers and a thumb. `size` px its palm's
// radius; `side` the way its thumb points (-1 left).
void palm(Raster& r, Point at, int size, int side, int shown) {
  if (shown <= 0) return;
  const int s = size;
  struct Finger {
    int dx, len;
  };
  const Finger fingers[4] = {
      {-s * 11 / 16, s * 20 / 16}, {-s * 4 / 16, s * 26 / 16}, {s * 4 / 16, s * 27 / 16}, {s * 11 / 16, s * 21 / 16}};
  const int width = s * 9 / 16;
  for (int pass = 0; pass < 2; ++pass) {
    const uint16_t ink = pass ? kGel : kRim;
    const int grow = pass ? 0 : 3;
    r.alpha = of(255, shown);
    r.disk(at.x, at.y, s + grow, ink);
    for (const Finger& f : fingers)
      r.line({at.x + side * f.dx, at.y - s / 3}, {at.x + side * f.dx * 9 / 8, at.y - f.len}, width + 2 * grow, ink);
    r.line({at.x - side * s * 13 / 16, at.y + s / 8}, {at.x - side * s * 25 / 16, at.y - s * 6 / 16}, width + 2 * grow,
           ink);
  }
  // Pressed flat: the heel paler, a shine on each fingertip.
  r.alpha = of(110, shown);
  r.disk(at.x, at.y + s / 8, s * 10 / 16, kShine);
  r.alpha = of(170, shown);
  for (const Finger& f : fingers) r.disk(at.x + side * f.dx * 9 / 8, at.y - f.len + 1, std::max(2, width / 4), kShine);
  r.alpha = 255;
}

// The label: one short word, big, rimmed dark to read over anything, its
// letters rippling a little, one after another.
void label(Raster& r, const char* word, int x, int y, int shown, int ms) {
  const Font& f = kSign;
  r.alpha = of(255, shown);
  int i = 0;
  for (const char* p = word; *p; ++i) {
    // One glyph at a time (a UTF-8 one whole), each with its own lift.
    int n = 1;
    while (p[n] && (p[n] & 0xC0) == 0x80) ++n;
    char one[8] = {};
    std::memcpy(one, p, size_t(std::min(n, 7)));
    p += n;
    const int gx = x + i * f.w, gy = y + 3 * sinTurn(ms * 4096 / 900 - i * 700) / 4096;
    for (int dy = -2; dy <= 2; dy += 2)
      for (int dx = -2; dx <= 2; dx += 2)
        if (dx || dy) text(r, f, gx + dx, gy + dy, one, kRim);
    text(r, f, gx, gy, one, kCream);
  }
  r.alpha = 255;
}

int glyphsIn(const char* s) {
  int n = 0;
  for (; *s; ++s) n += (*s & 0xC0) != 0x80;
  return n;
}

// Sparks flying out from (x, y), px: `n` four-pointed ones, `age` ms in.
void sparks(Raster& r, Point at, int from, int age, int life, int n) {
  if (age < 0 || age >= life) return;
  r.alpha = 255 - age * 255 / life;
  for (int i = 0; i < n; ++i) {
    const int a = i * 4096 / n + 256;
    const int d = from + age * from / life * 2, size = 6 - age * 3 / life;
    const int x = at.x + d * sinTurn(a + 1024) / 4096, y = at.y + d * sinTurn(a) / 4096;
    r.line({x - size, y}, {x + size, y}, 3, kSpark);
    r.line({x, y - size}, {x, y + size}, 3, kSpark);
  }
  r.alpha = 255;
}

// A heart about (x, y), px, `s` its half-width (render/gel/layers.cpp's).
void heart(Raster& r, int x, int y, int s, uint16_t ink) {
  const int x16 = x * 16, y16 = y * 16, half = s / 2 * 16, lobe = std::max(1, s / 2) * 16 + 8;
  r.disk16(x16 - half + 8, y16 + 8, lobe, ink);
  r.disk16(x16 + half + 8, y16 + 8, lobe, ink);
  const Point v[] = {{x16 - s * 16 + 8, y16 + 24}, {x16 + s * 16 + 8, y16 + 24}, {x16 + 8, y16 + (s + s / 2) * 16 + 8}};
  r.polygon16(v, 3, ink);
}

}  // namespace

int payoffAge(const InviteShow& v) {
  if (v.phase == Phase::kPayoff) return int(v.ms);
  if (v.phase == Phase::kLate && v.ms >= InviteShow::kTurnMs) return int(v.ms - InviteShow::kTurnMs);
  return -1;
}

void applyInvite(PaintControls& c, const Surface& s, const InviteShow& v) {
  if (!v.shows()) return;
  const int ms = int(v.ms), age = payoffAge(v), k = closeness(v), u = turned(v);
  const Point anchor = finishAnchor(s);
  c.anchorX = anchor.x, c.anchorY = anchor.y;
  c.scale = scaleOf(v);
  // The pose's lean, roll (its head tilted), squash, eyes up and wider, in
  // as it comes up to the glass. Turned away, it slumps, and its eyes come
  // round to you over its shoulder.
  int lean = 0, roll = 0, squash = 0, lift = 0, gaze = 0, up = 0, eyes = 0;
  const int w = v.pose == InvitePose::kBack ? u : k;
  switch (v.pose) {
    case InvitePose::kPalm: lean = of(700, w), roll = of(-140, w), eyes = of(450, w); break;
    case InvitePose::kLean:
      lean = of(-1500, w), roll = of(640, w), squash = of(600, w), up = of(520, w), eyes = of(300, w);
      break;
    case InvitePose::kBack: lean = of(600, w), squash = of(800, w), gaze = of(-280, w); break;
    default: break;
  }
  // The shrug: up and down, a glance aside under heavy lids.
  if (v.phase == Phase::kShrug) {
    const int b = bump(ms, 0, 900);
    lift += of(9, b), squash -= of(900, b), c.blink = std::max(c.blink, of(1500, b));
    gaze += of(of(320, ease(ms, 150, 450)), 4096 - ease(ms, 750, 1100));
  }
  // A late one turns round with a little hop.
  if (v.phase == Phase::kLate && ms < int(InviteShow::kTurnMs)) lift += of(12, bump(ms, 0, int(InviteShow::kTurnMs)));
  if (age >= 0) {
    switch (v.payoff) {
      case InvitePayoff::kHighFive:
        // The clap squashes it, then it hops, and wobbles round, pleased.
        squash += of(1300, bump(age, 0, 260)) - of(900, bump(age, 260, 560));
        lift += of(14, bump(age, 240, 720));
        lean += of(of(1100, sinTurn((age - 700) * 4096 / 650)), bump(age, 700, 2000));
        break;
      case InvitePayoff::kPurr:
        // A slow wiggle, eyes shut, breathing.
        lean += of(of(800, sinTurn(age * 4096 / 1200)), 4096 - ease(age, 1500, 2300));
        squash += of(220, sinTurn(age * 4096 / 600));
        break;
      case InvitePayoff::kSigh:
        // A breath in, tall; its shoulders drop; it settles.
        squash += of(1500, bump(age, 450, 1400)) - of(1000, bump(age, 0, 500));
        break;
      default: break;
    }
  }
  c.lean = std::clamp(c.lean + lean, -5120, 5120);
  c.roll += roll;
  c.squash = std::clamp(c.squash + squash, -5407, 5407);
  c.lift = std::clamp(c.lift + lift * 4096 / kPxPerUnit, 0, 983);
  c.gaze += gaze;
  c.gazeUp += up;
  c.eyeSize = of(c.eyeSize, 4096 + eyes);
  // Its face over its shoulder: squeezed toward its right rim.
  c.faceX += of(2150, u), c.faceW -= of(1850, u);
}

const char* inviteFace(const InviteShow& v) {
  const int age = payoffAge(v);
  if (v.phase == Phase::kLate && age < 0) return "happy";
  if (age >= 0) {
    switch (v.payoff) {
      case InvitePayoff::kHighFive: return "excited";
      case InvitePayoff::kPurr: return "affectionate";
      case InvitePayoff::kSigh: return age < 500 ? nullptr : "pleased";
      default: return nullptr;
    }
  }
  if (v.phase != Phase::kPose) return nullptr;
  switch (v.pose) {
    case InvitePose::kPalm: return "happy";
    case InvitePose::kLean: return "happy";
    default: return nullptr;
  }
}

Point inviteSpot(const InviteShow& v, const rig::Anchors& at) {
  const Point eyes{(at.eyeL.x + at.eyeR.x) / 2, (at.eyeL.y + at.eyeR.y) / 2};
  switch (v.pose) {
    case InvitePose::kPalm:  // held up at its right side, clear of its eye
      return {(at.eyeR.x + (at.eyeR.x - at.eyeL.x) * 7 / 16) / 16, (at.eyeR.y + at.mouth.y * 2) / 3 / 16 + 8};
    case InvitePose::kLean:  // its forehead, on the glass
      return {(at.crown.x * 3 + eyes.x * 2) / 5 / 16, (at.crown.y * 3 + eyes.y * 2) / 5 / 16};
    case InvitePose::kBack:  // its back, the side away from its face
      return {(at.brimL.x * 5 + at.brimR.x * 3) / 8 / 16, (at.brimF.y + at.handF.y) / 2 / 16};
    default: return {at.crown.x / 16, at.crown.y / 16};
  }
}

void paintInvite(Surface& s, const InviteShow& v, const rig::Anchors& at) {
  if (!v.shows() || !s.pixels) return;
  Raster r{s};
  const int ms = int(v.ms), age = payoffAge(v), scale = scaleOf(v);
  const Point spot = inviteSpot(v, at);
  const int radius = of(kGlowRadius, scale), shown = glowShown(v);
  // The glow, behind the palm; the palm, its heel pushed in as it claps.
  if (shown > 0) glow(r, spot, v.pose == InvitePose::kPalm ? radius * 5 / 4 : radius, shown, ms);
  const int push = age >= 0 && v.payoff == InvitePayoff::kHighFive ? bump(age, 0, 300) : 0;
  palm(r, spot, of(of(radius * 5 / 4, 4096 + of(900, push)), 4096 - 300 + of(300, palmShown(v))), 1, palmShown(v));
  if (shown > 0 && v.pose == InvitePose::kPalm) {
    const int breath = (sinTurn(ms * 4096 / kGlowBreathMs) + 4096) / 2;
    r.alpha = of(of(150, 2000 + of(2096, breath)), shown);
    circle(r, spot.x, spot.y, radius * 27 / 16, 3 * 16, kGlow);
    r.alpha = 255;
  }
  // The word by the glow: beside the palm, on the side away from the head's
  // tilt, under the glow on its back; inside the panel.
  if (v.label && shown > 0) {
    const Font& f = kSign;
    const int w = glyphsIn(v.label) * f.w, reach = radius * 2;
    int x = spot.x + reach, y = spot.y - f.h / 2;
    // Under the palm, and under the glow on its back.
    if (v.pose != InvitePose::kLean)
      x = spot.x - w / 2, y = spot.y + radius * (v.pose == InvitePose::kPalm ? 2 : 3) / 2;
    const int edge = s.inset(y, y + f.h) + 10;
    if (x + w > s.width - edge) x = spot.x - reach - w;
    x = std::clamp(x, edge, s.width - edge - w);
    y = std::min(y, s.height * 4 / 5 - f.h - 4);
    label(r, v.label, x, y, shown, ms);
  }
  if (age < 0) {
    // A late one, as it turns round: a pop of sparks by its head.
    if (v.phase == Phase::kLate) sparks(r, {at.crown.x / 16, at.crown.y / 16 + 10}, 24, ms - 100, 450, 5);
    return;
  }
  switch (v.payoff) {
    case InvitePayoff::kHighFive: {
      // A flash ring off the palm, and sparks flying.
      if (age < 420) {
        r.alpha = 230 - age * 230 / 420;
        circle(r, spot.x, spot.y, radius * 3 / 2 + age * radius * 2 / 420, 5 * 16, kWhite);
        r.alpha = 255;
      }
      sparks(r, spot, radius * 2, age, 800, 8);
      break;
    }
    case InvitePayoff::kPurr: {
      // Little hearts rising from its head, one after another.
      for (int i = 0; i < 3; ++i) {
        const int life = age - 150 - i * 500;
        if (life < 0 || life >= 1100) continue;
        r.alpha = std::min({255, life * 255 / 150, (1100 - life) * 255 / 400});
        const int x = spot.x + (i - 1) * 26 + 4 * sinTurn(life * 4096 / 700) / 4096, y = spot.y - 12 - life * 54 / 1100;
        heart(r, x, y, 10 + life * 4 / 1100, kHeart);
      }
      r.alpha = 255;
      break;
    }
    case InvitePayoff::kSigh: {
      // A puff of breath from its mouth, drifting off and thinning.
      const int life = age - 500;
      if (life < 0 || life >= 1200) break;
      const int mx = at.mouth.x / 16, my = at.mouth.y / 16;
      for (int i = 0; i < 3; ++i) {
        const int d = life * (30 + i * 16) / 1200;
        r.alpha = std::max(0, std::min(200, (1200 - life) * 200 / 600) - i * 40);
        r.disk(mx + 14 + d + i * 10, my - 4 - i * 6 - life * 10 / 1200, 5 + i * 2 + life * 4 / 1200, kCream);
      }
      r.alpha = 255;
      break;
    }
    default: break;
  }
}

LowSign inviteSign(const InviteShow& v) {
  LowSign l;
  const int age = payoffAge(v);
  if (age < 0 || v.xp <= 0) return l;
  static constexpr const char* kWords[] = {nullptr, "high five", "purr", "better now"};
  l.text = kWords[std::min(int(v.payoff), 3)];
  l.ink = rgb565(kEyeRgb);
  l.ms = uint32_t(age);
  l.leftMs = int32_t(InviteShow::kPayoffMs) - age;
  l.oneLine = true;
  l.xp = v.xp;
  return l;
}

InviteSound inviteSound(InvitePayoff p) {
  switch (p) {
    case InvitePayoff::kHighFive: return {"cheer", 150, 1000};
    case InvitePayoff::kPurr: return {"breath", 120, 700};
    case InvitePayoff::kSigh: return {"breath", 140, 850};
    default: return {};
  }
}

}  // namespace render::gel
