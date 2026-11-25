// The gel's invite (render::InviteShow, the `do`'s `invite`): after a skit it
// comes up to the glass, holds a pose, and the place to touch glows, a word
// by it; a touch there pays off.
//
// - palm: a hop, and one palm flat on the glass beside its right eye, the
//   glow round it: a tap is a high five, a clap and a ring, a hop and a
//   wobble, sparks off the palm. Missed, it looks at its palm, lowers it
//   and shrugs.
// - lean: it perks up and rests its cheek on the glass, the patch pale
//   where it touches, glowing: a stroke is a purr, a fast small wiggle,
//   eyes shut, little hearts rising. Missed, it eases off and shrugs.
// - back: it has turned its back and looks over its shoulder, its back
//   glowing: a stroke there is a sigh, its shoulders dropping as it turns
//   round, calmer, a puff of breath.
//
// The palm's and the lean's bodies and hands are the pack's rig clips
// (render/gel/moves.h); the back's, which the turn will take over, is
// drawn here in code. Light means "this is for you": the glow breathes and
// a ripple goes out from it now and then, so it reads as "touch here" with
// no word. A late touch (kLate) brings it back, pleased, into the payoff. A
// function of the InviteShow, the body's anchors and its hands alone, so
// the board, the simulator and the Mac's mirror draw the same.
#pragma once
#include <cstdint>

#include "render/face.h"
#include "render/gel/hands.h"
#include "render/gel/low_sign.h"
#include "render/gel/painter.h"
#include "render/gel/raster.h"
#include "render/rig/rig.h"
#include "render/surface.h"

namespace render::gel {

// A pose drawn in code comes up to the glass over kInviteInMs, and goes
// back over the last kInviteOutMs of a payoff.
constexpr int kInviteInMs = 450, kInviteOutMs = 600;
// How far up to the glass the back's pose comes (Q12 of the rig's glass).
constexpr int kBackGlass = 3600;
// The glow's radius about the place to touch, px at the body's own size,
// and its breath and its ripple's period.
constexpr int kGlowRadius = 30, kGlowBreathMs = 1400, kRippleMs = 1600;

// What a pose or payoff the pack has no clip for does to the body now (the
// back's): up to the glass (the controls' glass), and its lean, roll,
// squash, lift (canvas px), eyes and, turned away, its face over its
// shoulder; all on the engine's controls, as offsets. A clip's does nothing
// here.
void applyInvite(PaintControls& c, const Surface& s, const InviteShow& show);
// The face it wears now, by expression name: the pose's, the payoff's or a
// late turn's; null for the mood's own (the back pose, the shrug).
const char* inviteFace(const InviteShow& show);
// The place to touch, where it glows: px on the surface, the palm (the
// right hand, `hands[1]`), the cheek on the glass (the anchors' patch), or
// its back; and the glow's radius there.
Point inviteSpot(const InviteShow& show, const rig::Anchors& at, const Hand hands[2], int& radius);
// The glow and its ripple, behind a palm.
void paintInviteGlow(Surface& s, const InviteShow& show, Point spot, int radius);
// Over it all: a ring round the palm, the label, and the payoff's sparks,
// hearts or breath.
void paintInvite(Surface& s, const InviteShow& show, const rig::Anchors& at, Point spot, int radius);
// While a payoff earns XP: a low sign saying it, "+3 XP" on its chip.
LowSign inviteSign(const InviteShow& show);
// How far into its payoff it is at its time now, -1 before it (a late
// one's turn first).
int payoffAge(const InviteShow& show);
// The payoff's one sound, from the face's effect bank (render/gel/sounds.h):
// its recipe, how loud (of 255) and how high (permille).
struct InviteSound {
  const char* recipe = nullptr;
  uint8_t gain = 0;
  uint16_t pitch = 1000;
};
InviteSound inviteSound(InvitePayoff payoff);

}  // namespace render::gel
