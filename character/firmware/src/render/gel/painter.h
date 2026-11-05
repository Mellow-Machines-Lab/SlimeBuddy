#pragma once
#include <cstddef>
#include "render/rig/layers.h"
#include "render/rig/rig.h"
#include "render/stream.h"
#include "render/surface.h"
namespace render::gel {
// Q12 raster controls, fed by the deterministic engine.
struct PaintControls {
  int lean = 0, squash = 0, dent = 0, blink = 0, gaze = 0;
  int ripple = 0, lift = 0, roll = 0, forward = 0, grabX = 0, grabY = 0, dy = 0;
  int slide = 0;  // a gag's slide, and the rig's x: the whole dome, sideways
  // The rig's reach and melt (applyRig), Q12: one side stretched out, its
  // right for more than 0, and the other pinched in; and how far it has
  // melted down into a puddle, flatter and wider on the desk.
  int reach = 0, melt = 0;
  // A rig clip's eyes (applyRig): their size about their middles, Q12, and
  // how far up they look, in face units (how far across is in gaze).
  int eyeSize = 4096, gazeUp = 0;
  int mouthSize = 4096;  // the mouth's art, as eyeSize is the eyes'; both on top of `scale`
  // Pressed under the agents' drawer (render/gel/drawer.h): the rows the
  // body fills at rest (restBodyTop, restBodyBottom) mapped onto
  // [squeezeTop, squeezeBottom), in px, and spread wider by `widen` (Q12);
  // a squeezeBottom of 0 is none.
  int squeezeTop = 0, squeezeBottom = 0, widen = 4096;
  // A finish's (render/gel/finish.h): the whole body scaled (Q12) about
  // (anchorX, anchorY) and moved by (offsetX, offsetY), in sixteenths of a
  // pixel, its face and anchors with it; and its eyes left undrawn, for
  // the finish's own.
  int scale = 4096, anchorX = 0, anchorY = 0, offsetX = 0, offsetY = 0;
  bool eyes = true;
};
// A growth stage's body, on every animation, as the stage's tag says
// (render/tags.h): its size, about where it stands on the desk, and its
// eyes' size about their middles and its mouth's, Q12, and how far it's
// squashed (as PaintControls' squash). Every stage draws the same art; Buddy, with no tag,
// is the size the art is laid out at. The Chonk is a wider Buddy, about as
// tall as ever: the hats, the sign over its head and the needs-you card
// have no room above it to grow into.
struct BodySettings {
  int size = 4096, eyes = 4096, squash = 0, mouth = 4096;
};
constexpr BodySettings kHatchlingBody{2253, 6349, 0, 5734}, kSproutBody{2867, 5120, 0, 4915}, kBuddyBody{},
    kChonkBody{4588, 4096, -867};
BodySettings bodySettings(uint32_t when);
// The hat it wears now: as the tags say (render/tags.h), but a Sprout's leaves
// are its party: a party hat would hide them on the very day they came.
Outfit wornHat(uint32_t when);
// How far the nose sinks at these settings, on the canvas's 240 rows: it
// stands this far above where the body stands at Buddy's height.
constexpr int kNoseOverDesk = 54;
// How tall it stands at these settings, Q12 of Buddy's.
constexpr int heightOf(const BodySettings& b) { return b.size * (4096 + b.squash * 2130 / 4096) / 4096; }
constexpr int noseDropFor(const BodySettings& b) { return kNoseOverDesk - kNoseOverDesk * heightOf(b) / 4096; }
// The canvas row the body stands on (restBodyBottom), and how far above it
// its eyes are, at Buddy's height.
constexpr int kDeskRow = 188, kEyesOverDesk = 50;
// Changing stage. Growing: it glows and draws in, pops past its new size and
// settles. Shrinking: it sinks, with no show. The first tags it's told only
// say what it is: it isn't growing up every time the Mac connects. A
// function of the engine's clock, so the board, the simulator and the mirror
// draw the same frame.
constexpr int kGrowGlowMs = 900, kGrowPopMs = 260, kGrowSettleMs = 520, kShrinkMs = 1600;
// How far under its old size it draws in, and past its new one it pops, in Q12.
constexpr int kGrowDraw = 3768, kGrowPast = 4588;
struct BodyChange {
  BodySettings from, to;
  int stage = 3;  // growthStage's, of `to`
  uint32_t at = 0;
  bool told = false, moving = false, growing = false;
  void see(uint32_t when, uint32_t now);
  // Ends a change that's over, from every step: see() may be held back, and
  // an old `at` would read as the future once the clock's gone round.
  void settle(uint32_t now) {
    if (moving && int32_t(now - at) >= (growing ? kGrowGlowMs + kGrowPopMs + kGrowSettleMs : kShrinkMs)) moving = false;
  }
  BodySettings shown(uint32_t now) const;
  // Whether it's at its new stage yet: a stage's feature comes with the pop.
  bool arrived(uint32_t now) const { return !moving || !growing || int32_t(now - at) >= kGrowGlowMs; }
  // How much it glows, and how far the burst is through, 0 … 255: both 0 out
  // of a growth.
  int glow(uint32_t now) const;
  int burst(uint32_t now) const;
};
// The rows the body fills at rest, laid out on the AMOLED's 410.
inline int restBodyTop(int height) { return height * 92 / 410; }
inline int restBodyBottom(int height) { return height * 322 / 410; }
// Where the body is this frame, from the same pose that draws it: the rig's
// anchors (render/rig/rig.h), in sixteenths of a pixel, which a layer
// (render/gel/layers.h) sits on, so it moves, squashes and leaves the screen
// with the body; the face art adds the eyes' middles. The brim's, the
// hands', the lap's and the hem's are on these rings of the mesh, from the
// top (0) down: the hem's is the lowest the camera sees at the front.
constexpr int kBrimRing = 6, kHandRing = 15, kLapRing = 14, kHemRing = 17;
struct PastimeShow;
// The body; the surface's clear first keeps what this frame covers again
// (Surface::clear), and the pastimes' scenery goes behind it
// (render/gel/pastimes.h), when given. Its anchors into `anchors`, when
// given.
void paint(Surface& surface, const PaintControls& controls, const Box* keep = nullptr, int keepCount = 0,
           uint32_t keepLane = 0, rig::Anchors* anchors = nullptr, const PastimeShow* scenery = nullptr);
// The body alone, over what the surface holds.
void paintBody(Surface& surface, const PaintControls& controls, rig::Anchors* anchors = nullptr);
// The body's material, the texels it's drawn from, as linked: and a copy
// of them to read instead, made where they're quicker to read (on the
// AMOLED, PSRAM over flash: GelFace::readAssetsFrom), or none for these.
const void* materialTexels(size_t& bytes);
void readMaterialFrom(const void* copy);
// The dome on the canvas: canvas pixels a body unit at its middle.
constexpr int kPxPerUnit = 85;
// The rig's gazeX and gazeY at their whole (1000) move the eyes this far,
// in face units (Q12 of a body unit); its mouthOpen from kRigMouthOpen
// opens the gel's one open mouth.
constexpr int kRigGaze = 205, kRigMouthOpen = 300;
// The rig's channels (render/rig/), what its clips play on the gel, as
// offsets from rest on the controls the engine gives: the body's squash
// (the rig's tall is the gel's up), lean, lift (canvas px, kPxPerUnit a
// body unit, never into the floor: the whole dome rises), x (canvas px,
// the whole dome sideways, off the screen if far enough), reach and melt,
// the lids, shut as far as the more shut eye, and the eyes' size and
// where they look. The gel draws no other channel yet but the mouth
// (paint). At rest, none moves.
void applyRig(PaintControls& c, const rig::Channels& rig);
class Engine;
// The body, the face art, what it wears and the air around it
// (render/gel/layers.h) as the layer stack shows them at t (whole in the
// engine's tags and state with none), the pastimes' kit it wears, then the
// props, with the rig's channels on the engine's controls, and the
// pastimes' scenery behind it all; a lane that shows `keepLane` is kept
// from this surface's last frame. Its anchors into `anchors`.
// Pressed under the agents' drawer (`squeeze`, render/gel/drawer.h), it
// draws flattened, and with its eyes squeezed shut it draws the body alone.
struct DrawerPose;
// A work act (render/gel/act.h) moves the body as a finish does, and draws
// behind it and over it.
void paint(Surface&, const Engine&, bool mouthOpen = false, int dy = 0, uint32_t keepLane = 0,
           rig::Anchors* anchors = nullptr, const rig::Channels& rig = rig::kRest, const rig::Layers* layers = nullptr,
           uint32_t t = 0, const PastimeShow* pastimes = nullptr, const DrawerPose* squeeze = nullptr,
           const Stream::ActNow* act = nullptr);
void paintArt(Surface&, const Engine&, const PaintControls&, bool mouthOpen, rig::Anchors* anchors = nullptr);
// Hushed (the `hushed` tag, render/tags.h), its lips are zipped: in the
// mouth's place, in its ink and following the face as it does, whatever
// mouth its art has or a clip opens, a short closed line kZipHalf face
// units (Q12) either side of the middle, with kZipTicks little ticks
// across it kZipTick either side, the line's pen kZipPen and the ticks'
// kZipTickPen in the art's widths (paintZip).
constexpr int kZipHalf = 270, kZipTicks = 3, kZipTick = 85, kZipPen = 80, kZipTickPen = 48;
bool hushed(const Engine&);
void paintZip(Surface&, const Engine&, const PaintControls&);
// The most room a cross-fading group of the art has taken (kCrossingBytes).
extern int crossingPeak;
// The desk's paws are where a pastime puts them (render/gel/pastimes.h),
// typing with none.
struct Paws;
void paintProps(Surface&, const Engine&, const Paws* paws = nullptr);
// What a stage wears, over the body and what's on it: a Sprout's sprout on
// its crown, kept off under a hat. And changing stage, the glow and the burst.
void paintSprout(Surface&, const Engine&, const rig::Anchors&, int scale);
void paintGrowing(Surface&, const Engine&, const rig::Anchors&);
// A paw types: before each of its key contacts in the recipe playing it
// lifts and comes back down over kTapMs, kTapLift thousandths of the
// surface's height at the top, landing on the contact (whose sound and
// dip it is). How far up the paw on `side` (-1 left, 1 right) is now.
constexpr int kTapMs = 180, kTapLift = 24;
struct Scenes;
int tapLift(const Scenes& scenes, int side);
// Between them it types on softly, without a sound: a tap every kTypeMs,
// the paws by turns, each lifting kTypeLift over kTapMs, at `ms` of the
// engine's clock.
constexpr int kTypeMs = 340, kTypeLift = 12;
int typeLift(uint32_t ms, int side);
// The cards paintProps will fill opaque this frame, as boxes of the pixels
// it covers whole (at most Surface::kMaxKeep), for the clear to leave.
int opaqueCards(const Surface&, const Engine&, Box* out);
}  // namespace render::gel
