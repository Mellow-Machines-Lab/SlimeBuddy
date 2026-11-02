// The low sign: a card Boop holds up in the strip's place, its words in two
// lines at most, the whole face clear above it. It holds a name too long
// for a reaction's sign (render/gel/layers.h), which crosses the lower face.
// It's the lane's to draw: it stays inside the lane's rows, so a frame whose
// sign hasn't moved keeps them and pushes nothing there.
// A function of the LowSign alone, so the board, the simulator and the Mac's
// mirror draw the same frame.
#pragma once
#include <cstdint>

#include "render/surface.h"

namespace render::gel {

struct LowSign {
  enum class Mark : uint8_t { kNone, kTick, kCross, kDots };
  const char* text = nullptr;  // none: no sign
  Mark mark = Mark::kNone;     // before the words, in `ink`
  uint16_t ink = 0;            // its rim, its mark and its words
  uint32_t ms = 0;             // how long since it began to rise
  int32_t leftMs = -1;         // how long until it has fallen away, -1 for no end yet
  bool shows() const { return text && *text; }
};

// It rises over this long, and falls away over the last of its leftMs.
constexpr int kLowSignRiseMs = 280, kLowSignFallMs = 400;
// The most glyphs it holds, two lines of the small size; a longer text is cut
// as render::cutAt cuts it.
constexpr int kLowSignMostGlyphs = 50;

// How it lays out `text` on a surface: the font's size, its lines and the
// card, at rest. Pure, for the tests.
struct LowSignLayout {
  static constexpr int kLineBytes = 2 * kLowSignMostGlyphs + 1;
  bool large = false;  // the sign's font, for a short name; else the bubble's
  int lines = 0;
  char line[2][kLineBytes] = {};
  int x = 0, y = 0, w = 0, h = 0;
};
LowSignLayout lowSignLayout(int width, int height, int corner, const char* text, bool marked);

// How far below its place it is now: 0 at rest.
int lowSignDrop(const LowSign& sign, int height);
// What it shows, as a key that's the same exactly when its pixels are, never 0.
uint32_t lowSignKey(const LowSign& sign);
// The sign, over whatever the lane drew.
void paintLowSign(Surface& surface, const LowSign& sign);

}  // namespace render::gel
