// The work acts on the gel (render/stream.h's Stream::ActNow): a command Boop
// plays out, with the low sign saying what's running. A function of how long
// the act has run, so the board, the simulator and the Mac's mirror draw the
// same frame.
//
// - The push: on a rocket, it blasts off the screen; only the trail and the
//   sign stay, and now and then it drifts by in orbit. It parachutes down
//   proud, or crash-lands in soot with the rocket in the ground.
// - The tests: a crash-test dummy in a helmet with targets, strapped to a
//   sled at a start line, facing a wall, puffed up and holding its breath, the
//   sled inching on. It stops short with a big phew and a thumbs up, or
//   hits the wall, splats and slides down it.
// - The build: in a hard hat, hammering in a loop with sparks, a little house
//   going up beside it course by course. The house is done, with a flag, and
//   it looks at it, proud; or the house falls apart, its hat tilted, its
//   hammer down.
#pragma once
#include "render/gel/finish.h"
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

// The tests' opening: the wall and the sled come, then it holds its breath.
constexpr int kTestsSetMs = 900;
// How far along the track the sled gets, ever: it inches, and slows.
// A pass's stop, and a fail's rush to the wall and its slide down it.
constexpr int kPassStopMs = 300, kSplatMs = 300, kSlideMs = 2200;

// The build: a swing of the hammer, and how long the house takes to go up.
constexpr int kBuildSetMs = 700, kSwingMs = 760, kHouseMs = 75000;

// What it does to the body now: as a finish's pose, about its anchor
// (render/gel/finish.h).
struct ActPose : FinishPose {
  bool away = false;  // not on the screen at all
};
ActPose actPose(const Surface& s, const ActShow& act);
// A full act holds the screen: no desk, no pastime, no reaction's sign.
inline bool actTakesOver(const ActShow& act) { return act.shows() && act.full; }
// What the act draws behind the body and over it, at the body's anchors. The
// small version draws only its prop, over it, as it does its Doing.
// What an act draws behind the body, or its still part kept (`kept`): a
// box of it that holds the same opaque pixels from frame to frame, if there
// is one, and a key to say so (Surface::stillKey), else 0.
uint32_t actKeeps(const Surface& s, const ActShow& act, Box& box);
void paintActBehind(Surface& s, const ActShow& act, bool kept);
void paintActFront(Surface& s, const ActShow& act, const rig::Anchors& at);
// The face it wears now, or -1 for its mood's. A cross or low mood keeps its
// own (`mood`, the engine's emotion).
int actFace(const ActShow& act, int mood);
// The low sign that says what's running, then how it ended.
LowSign actSign(const ActShow& act);

}  // namespace render::gel
