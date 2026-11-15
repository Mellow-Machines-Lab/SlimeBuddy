// What the gel wears and holds over its body, drawn natively by coverage
// like the rest of the gel, at the anchors of the pose that drew the body
// (the rig's Anchors, render/gel/painter.h), so each follows the body as it
// moves, squashes and leaves the screen, and is cut off there with it.
//
// THE SCREEN'S RULE, part one: the face comes first, and what it wears is
// on its body, one thing at a time. Nothing hangs in the sky behind it and
// nothing drifts through the air round it for a time of day, a season or a
// holiday: no sun, moon or stars, no snow, confetti or hearts, no coffee.
// A day shows as one thing worn, and nothing else:
//
// - the outfit, by the tags on now (state.when): a striped nightcap at
//   night (and no pyjamas: one thing worn), a party hat on a
//   party day, New Year's Day, Lunar New Year and Programmers' Day, a
//   witch hat at Halloween, a bobble hat in winter; shades on a Friday's
//   day, off while it sleeps;
// - beside it: Zs rising while it sleeps at night;
// - the sign a reaction holds up (its `sign`): a dark card held at its
//   front, under its mouth, with a little gel hand at either side, the words
//   on it, sliding up as the reaction starts and bobbing while it's held. A
//   small body rises to stand on it, so the card leaves its face clear (signHop).
//
// Part two, what may move beside the face, is in render/gel/pastimes.h;
// part three, where its eyes look, in render/gel/gaze.h; what shares the
// top band and the lane, in render/gel/lane.cpp.
//
// Each comes and goes through the rig's layer stack (render/rig/layers.h,
// the face's), so a hat at midnight, the Zs as it falls
// asleep and shades on a Friday each fade over the rig's fade, and a new
// one waits for the old to go; the sign fades out at the opacity Behaviour
// gives it. A scene may hide a layer: peekaboo's gesture takes the outfit
// off while the slime comes up from nothing. At night its lids also hang a
// little lower (sleepyLids).
#pragma once
#include "render/face.h"
#include "render/gel/painter.h"
#include "render/rig/layers.h"
#include "render/surface.h"

namespace render::gel {

// Each part of the outfit (the hat, riding the brim's front; what's over
// its eyes, riding the left eye) is drawn into a recording of
// its own (render/gel/raster.h) when its look changes or the body has
// changed its shape (its anchors moved apart from the one it rides) by
// more than kSlack sixteenths of a pixel, and blended from it in every
// other frame, moved to the sixteenth with the anchor it rides
// (Raster::replay16): their filled area is most of what the layers cost.
// While the body moves (a frame of the same body up to kMovingMs before,
// and its anchors moved within kStillMs), a frame may show a part in the
// shape it was kept, within kSlack of its own; still, it's drawn where it
// is unless it moved whole pixels in the same shape, so a still frame is
// the same as drawing it afresh. The nightcap's swing steps every kSwingMs, as
// Pixel's pompom does. A part is kept in a frame as wide as the surface
// and kFrameHeight tall, with the anchor it rides kFrameY down its middle:
// room around for any of them. Past its edges, or the recording's room,
// it's drawn straight on the surface.
struct OutfitLayer {
  static constexpr int kSlack = 12, kSwingMs = 400;
  static constexpr uint32_t kMovingMs = 100, kStillMs = 200;
  static constexpr int kFrameY = 288, kFrameHeight = 512;
  static constexpr int kHatBytes = 7 * 1024, kWornBytes = 10 * 1024;  // the hat's room, and the eyewear's
};

struct SignLayer {
  static constexpr int kPad = 20;                 // the words from the card's sides, clear of the hands
  static constexpr int kGap = 4;                  // the card's bottom over the lane, at the lowest
  static constexpr int kRisePx = 90, kBobPx = 3;  // its signLift
  static constexpr int kHand = 12;                // the radius of the hand gripping either side
  static constexpr int kMinWide = 120;            // the narrowest card, hands' middles to middles
  // The hands' outer edges from the panel's sides, and clear of its rounded
  // corners, at the nearest: a lean or a finish carrying the body sideways
  // carries the sign only as far.
  static constexpr int kEdge = 8;
  // How far a Hatchling and a Buddy rise to stand on their sign: its face is
  // low on a small body, and the card, which can't go into the lane, would
  // cover it. A Sprout's between them; a Chonk's mouth clears it standing.
  static constexpr int kHopPx = 64, kBuddyHopPx = 14;
};

// How far its lids hang at night (Q12, as PaintControls::blink): 0 by day,
// and on the sleeping face, whose eyes are shut anyway: easing out as the
// awake face squashes shut, and back as it wakes.
constexpr int kNightLids = 1100;
class Engine;
int sleepyLids(const Engine& e);

// What the layer stack's layers (render/rig/layers.h) show for the
// engine's tags and state now: what it wears over its eyes or on its body
// (worn) and its hat, each a content of its own (0 for none), and beside
// it the Zs while it sleeps at night. The stack's scene and air layers
// stay empty: the screen's rule, above.
enum : uint32_t { kZs = 1 };
struct Contents {
  uint32_t worn, hat, beside;
};
Contents layerContents(const Engine& e);
// The outfit and the Zs, over the body and its face, under the props: what
// the stack shows at t, at its layers' opacities; with none, whole in what
// the engine's tags and state call for.
void paintLayers(Surface& surface, const Engine& e, const rig::Anchors& at, const rig::Layers* layers = nullptr,
                 uint32_t t = 0);
// How far the card sits below its place `ms` after it went up: sliding up,
// then bobbing.
int signOffset(uint32_t ms);
// The sign, at the body's front (its hands' ring), over everything but
// the lane.
void paintSign(Surface& surface, const SignShow& sign, const rig::Anchors& at, int hop = 0);
// How far the body rises for its sign now (SignLayer::kHopPx), easing up with
// the card and down as it fades.
int signHop(const SignShow& sign, const BodySettings& body);
// The wish: a small thought bubble beside the head with its icon (a hand,
// a speech dot, a laptop); met, the icon pops into a heart with sparks; left,
// the bubble fades.
void paintWish(Surface& surface, const WishShow& wish, const rig::Anchors& at);
// The growth bar along the top of the screen: this stage's little Boop, the
// bar, and the next stage's in shadow, as the popover shows it; full and gold
// at the last stage. A met wish's heart flies from its bubble into the bar's
// tip, and the bar gulps. Its row is clear of Buddy's head, under what's worn.
// `yield` (0–255): how far it has given way to what else holds the top band.
void paintGrowthBar(Surface& surface, const GrowthShow& bar, const rig::Anchors& at, bool hat = false, int yield = 255);

}  // namespace render::gel
