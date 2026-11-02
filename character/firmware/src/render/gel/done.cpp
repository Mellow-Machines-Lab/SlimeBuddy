#include "render/gel/done.h"

#include <algorithm>

#include "render/gel/raster.h"
#include "render/maths.h"
#include "render/palette.h"

namespace render::gel {
namespace {

uint16_t inkOf(Outcome o) {
  return o == Outcome::kSuccess ? rgb565(kSageRgb) : o == Outcome::kFailure ? rgb565(kClayRgb) : rgb565(kEyeRgb);
}

uint32_t isqrt(uint32_t v) {
  uint32_t root = 0, bit = 1u << 30;
  while (bit > v) bit >>= 2;
  while (bit) {
    if (v >= root + bit) {
      v -= root + bit;
      root = (root >> 1) + bit;
    } else {
      root >>= 1;
    }
    bit >>= 2;
  }
  return root;
}

}  // namespace

int doneGlow(const DoneShow& d) {
  if (!d.shows()) return 0;
  const int peak = d.outcome == Outcome::kNone ? kGlowPeakFaint : kGlowPeak;
  int g = peak * ease(int(std::min<uint32_t>(d.ms, kGlowInMs)), kGlowInMs) / 1024;
  if (d.leftMs >= 0) g = g * ease(std::min(int(d.leftMs), kGlowOutMs), kGlowOutMs) / 1024;
  return g;
}

LowSign finishSign(const DoneShow& d) {
  LowSign sign;
  if (!d.shows() || d.signMs < 0) return sign;
  sign.text = d.thread && *d.thread ? d.thread : d.agent;
  sign.mark = d.outcome == Outcome::kSuccess   ? LowSign::Mark::kTick
              : d.outcome == Outcome::kFailure ? LowSign::Mark::kCross
                                               : LowSign::Mark::kDots;
  sign.ink = inkOf(d.outcome);
  sign.ms = uint32_t(d.signMs);
  sign.leftMs = d.leftMs;
  return sign;
}

// Each pixel takes the outcome's ink by how far in from the panel's edge
// it is, d: (1 - d / kGlowPx)², strongest at the edge. Round the corners
// that's how far in from their arcs; elsewhere from the nearer edge. A row
// holds it only within kGlowPx of an edge, or of a corner's arc.
void paintGlow(Surface& s, const DoneShow& d, int from, int to) {
  const int peak = doneGlow(d);
  if (!s.pixels || peak <= 2) return;
  const uint16_t ink = inkOf(d.outcome);
  const int w = s.width, h = s.height, reach = kGlowPx * 16;
  const int radius = std::clamp(s.corner, 0, std::min(w, h) / 2), r16 = radius * 16;
  uint8_t alphaOf[kGlowPx * 16];
  for (int i = 0; i < reach; ++i) alphaOf[i] = uint8_t(peak * (reach - i) * (reach - i) / (reach * reach));
  // How far in a pixel is, in sixteenths, from its middle; past the reach,
  // or outside the panel, none.
  auto alphaAt = [&](int x, int ey16) {
    const int xc = x * 16 + 8, ex16 = std::min(xc, w * 16 - xc);
    int in;
    if (ex16 < r16 && ey16 < r16) {
      const int cx = r16 - ex16, cy = r16 - ey16;
      in = r16 - int(isqrt(uint32_t(cx * cx + cy * cy)));
    } else {
      in = std::min(ex16, ey16);
    }
    return in < 0 || in >= reach ? 0 : int(alphaOf[in]);
  };
  auto blend = [&](uint16_t* px, int a) {
    if (a > 2) *px = mix565(*px, ink, a);
  };
  for (int y = std::max(0, from); y < std::min(h, to); ++y) {
    const int yc = y * 16 + 8, ey16 = std::min(yc, h * 16 - yc);
    uint16_t* row = s.pixels + y * w;
    // How far in the panel's edge is on this row, past its corners.
    int edge = 0;
    if (ey16 < r16) {
      const int cy = r16 - ey16;
      edge = (r16 - int(isqrt(uint32_t(r16 * r16 - cy * cy)))) / 16;
    }
    if (ey16 < reach) {
      // Near the top or the bottom: all of it, the middle at one strength.
      const int a = alphaOf[ey16];
      for (int x = 0; x < radius; ++x) blend(row + x, alphaAt(x, ey16)), blend(row + w - 1 - x, alphaAt(x, ey16));
      if (a > 2) Blender(ink, a).run(row + radius, w - 2 * radius);
      s.touch(0, y, w, y + 1);
      continue;
    }
    const int band = std::min(w / 2, edge + kGlowPx + 1);
    for (int x = 0; x < band; ++x) {
      const int a = alphaAt(x, ey16);
      blend(row + x, a), blend(row + w - 1 - x, a);
    }
    s.touch(0, y, band, y + 1);
    s.touch(w - band, y, w, y + 1);
  }
}

}  // namespace render::gel
