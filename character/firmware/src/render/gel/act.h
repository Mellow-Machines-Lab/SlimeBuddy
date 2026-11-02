// The work acts on the gel (render/stream.h's Stream::ActNow): a command Boop
// plays out, with the low sign saying what's running. A function of how long
// the act has run, so the board, the simulator and the Mac's mirror draw the
// same frame.
//
// - The push: on a rocket, it blasts off the screen; only the trail and the
//   sign stay, and now and then it drifts by in orbit. It parachutes down
//   proud, or crash-lands in soot with the rocket in the ground.
#pragma once
#include "render/gel/low_sign.h"
#include "render/rig/rig.h"
#include "render/stream.h"
#include "render/surface.h"

namespace render::gel {

using ActShow = Stream::ActNow;

// The opening's beats, from the act's start: the rocket comes up under it,
// it lights, it lifts, and it's gone.
constexpr int kPushBoardMs = 800, kPushLightMs = 1700, kPushGoneMs = 3200;
// In orbit: the first pass this long in, then one every kOrbitEveryMs, each
// taking kOrbitMs to cross the screen.
constexpr int kOrbitFirstMs = 20000, kOrbitEveryMs = 26000, kOrbitMs = 8000;
// A parachute's landing and a crash's impact, from the result's start.
constexpr int kChuteLandMs = 2500, kCrashHitMs = 520;

// What it does to the body now: as a finish's pose (render/gel/finish.h).
struct ActPose {
  int dx16 = 0, dy16 = 0, scale = 4096, squash = 0, lean = 0;
  bool away = false;  // not on the screen at all
};
ActPose actPose(const Surface& s, const ActShow& act);
// Where it scales the body about: the middle of where it stands.
rig::Point actAnchor(const Surface& s);
// A full act holds the screen: no desk, no pastime, no reaction's sign.
inline bool actTakesOver(const ActShow& act) { return act.shows() && act.full; }
// What the act draws behind the body and over it, at the body's anchors.
void paintActBehind(Surface& s, const ActShow& act);
void paintActFront(Surface& s, const ActShow& act, const rig::Anchors& at);
// The face it wears now, or -1 for its mood's. A cross or low mood keeps its
// own (`mood`, the engine's emotion).
int actFace(const ActShow& act, int mood);
// The low sign that says what's running, then how it ended.
LowSign actSign(const ActShow& act);

}  // namespace render::gel
