// The gel's invite (render::InviteShow, the `do`'s `invite`): after a skit it
// turns to you, comes up to the glass, holds a pose, and the place to touch
// glows, a word by it; a touch there pays off.
//
// - palm: it faces you, close, one palm flat on the glass, glowing: a tap
//   there is a high five, a clap and a hop with sparks off the palm.
// - lean: it faces you, close, its head leaned on the glass and its eyes up
//   at you, its forehead glowing: a stroke there is a purr, its eyes shut
//   in a slow wiggle, little hearts rising.
// - back: it has turned its back and looks over its shoulder, its back
//   glowing: a stroke there is a sigh, its shoulders dropping as it turns
//   round, calmer, a puff of breath.
//
// Light means "this is for you": the glow breathes and a ripple goes out
// from it now and then, so it reads as "touch here" with no word. Missed,
// it shrugs and turns back; a touch after (kLate) turns it round, pleased,
// into the payoff. A function of the InviteShow and the body's anchors
// alone, so the board, the simulator and the Mac's mirror draw the same.
#pragma once
#include <cstdint>

#include "render/face.h"
#include "render/gel/low_sign.h"
#include "render/gel/painter.h"
#include "render/gel/raster.h"
#include "render/rig/rig.h"
#include "render/surface.h"

namespace render::gel {

// It comes up to the glass over kInviteInMs, and goes back over the last
// kInviteOutMs of a payoff.
constexpr int kInviteInMs = 450, kInviteOutMs = 600;
// How much bigger it is at the glass (Q12), by pose.
constexpr int kInviteScale[] = {4096, 4870, 5160, 4630};
// The glow's radius about the place to touch, px at the body's scale, and
// its breath and its ripple's period.
constexpr int kGlowRadius = 30, kGlowBreathMs = 1400, kRippleMs = 1600;

// What the invite does to the body now: its scale up to the glass (about
// where it stands, as a finish's), and the pose's lean, roll, squash, lift
// (canvas px), eyes and, turned away, its face over its shoulder; all on
// the engine's controls, as offsets.
void applyInvite(PaintControls& c, const Surface& s, const InviteShow& show);
// The face it wears now, by expression name: the pose's, the payoff's or a
// late turn's; null for the mood's own (the back pose, the shrug).
const char* inviteFace(const InviteShow& show);
// The place to touch, where it glows: px on the surface, at the body's
// anchors (the palm, the forehead, its back).
Point inviteSpot(const InviteShow& show, const rig::Anchors& at);
// The palm, the glow and its ripple, the label, and the payoff's sparks,
// hearts or breath, over the body and its face.
void paintInvite(Surface& s, const InviteShow& show, const rig::Anchors& at);
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
