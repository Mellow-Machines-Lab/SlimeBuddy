// The finished turn the brain's reaction is about (render::DoneShow), as
// the gel marks it: the panel's rim glowing in its outcome's colour, and,
// once its words have ended, its thread's name held up on the low sign
// (render/gel/low_sign.h). The glow comes as the reaction starts; both go
// once it has lingered (app/behaviour.h), the glow fading out over its last
// second, the sign dropping away.
// Integer, and a function of the DoneShow alone, so the board, the
// simulator and the Mac's mirror draw the same frame.
#pragma once
#include "render/face.h"
#include "render/gel/low_sign.h"
#include "render/surface.h"

namespace render::gel {

// How long the glow takes to come, and to fade as the finished turn goes.
constexpr int kGlowInMs = 300, kGlowOutMs = 1000;
// How far in from the panel's edge the glow reaches, in px, and how strong
// it is at the edge (0–255): a reply's, or a face's with no finish of its
// own, fainter.
constexpr int kGlowPx = 26, kGlowPeak = 200, kGlowPeakFaint = 110;

// The glow's strength at the panel's edge, 0–255: 0 for none.
int doneGlow(const DoneShow& done);
// The low sign that names it: none until it's up.
LowSign finishSign(const DoneShow& done);
// The glow, on rows [from, to), over whatever's drawn there.
void paintGlow(Surface& surface, const DoneShow& done, int from, int to);

}  // namespace render::gel
