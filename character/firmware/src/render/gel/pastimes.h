// What the gel does to pass the time: its pastimes, a little scene each,
// and the kit the tiers put out round it, so something new shows at every
// glance however long the agents work or you're away.
//
// The pastimes are one table (kPastimes), a row each, and the face deals
// them all (GelFace), with dice of its own. A row's body is one of three:
// one of the pack's rig clips (rig/clips/: a sip, a hop, a lean to look)
// and what the board draws with it, in time with it (the mug coming up to
// its mouth, the bubble it blows, the book it reads); a bit of the engine's
// (a yawn, a sneeze, a holiday's bounce: slime::kGags, by name), which its
// acting plays in place of its next gesture; or a take of the engine's
// (busier typing, a deeper snore, a dream: slime::kPastimeTakes, by name),
// which its scene plays as the look's next take. Each is for one look:
// working, idle, needs you or asleep; a take at work only while it types.
// A working or idle one needs a tier (render/tags.h, `tier1` from 30
// seconds to `tier5` from 10 minutes: the Mac's clock of the agents'
// working stretch or of their rest), and one while something needs you or
// asleep, a wait, the seconds its look has shown (the needs-you screen's
// from the request); a row that needs a tag (a holiday, the night, busy)
// needs no tier: it fits whenever its tag is on. They're dealt one at a time, a rest of kRestMinMs to kRestMaxMs
// between them, the newest kNewestPct of the time and an older one's the
// rest, never the same twice in a row: the newest are those of the
// highest tier, then the longest wait, and a row that needs a tag is
// newer than any while it fits. A few are moves, the whole body going
// somewhere (reaching for its coffee, melting, a stretch break off the
// screen), rarer than the rest: never within kMoveGapMs of the last.
// While a pastime moves the desk's paws (Enter pressed, the hands up at
// its head, spread as it melts, away with it), they're where it puts them
// (pawsFor).
//
// THE SCREEN'S RULE, part two (part one is render/gel/layers.h): beside
// the face, at most one thing moves at a time, and nothing stands about
// for no reason. So the kit is one thing, worn: headphones at work from
// tier3, the hat winning where it has one, put on where you can see it
// (kPhonesOn, the first pastime dealt once they're wanted), so they say it
// has settled in, and on from then while they're wanted. No mug on the
// desk or stack of them, no motes "in the zone", no plant, and no notes
// floating off the headphones: each was a second or third thing beside
// the face that had to be explained. A pastime brings what it holds and
// takes it away again, and they're dealt one at a time with a long rest
// between (kRestMinMs), never while a word of the work stream is forward,
// a work act plays or a finger is on it (GelFace::passTime).
#pragma once
#include <cstdint>

#include "render/rig/rig.h"
#include "render/surface.h"
#include "render/types.h"

namespace render::gel {

class Engine;

enum class Pastime : uint8_t {
  kNone,
  // At work.
  kPeek,
  kHum,
  kDone,
  kPonder,
  kRead,
  kNod,
  kBop,
  kDrum,
  kSticky,
  kPhonesOn,
  kSnack,
  kMelt,
  kStretch,
  kLimber,
  kBusyTyping,
  kFranticTyping,
  // Idle.
  kLookAround,
  kHopeful,
  kHop,
  kBubble,
  kDoodle,
  kBook,
  kGame,
  kBrb,
  kTower,
  kDance,
  kPlane,
  kSnooze,
  kDaydream,
  kSnuggle,
  kYawn,
  kSneeze,
  kMorningStretch,
  kNightDrowse,
  kPartyBounce,
  kFridaySway,
  kWinterShiver,
  kHalloweenWobble,
  kValentinesSwoon,
  // While something needs you.
  kWave,
  kTap,
  kWatch,
  kDrowsy,
  // Asleep.
  kRollOver,
  kSheep,
  kSnoreBubble,
  kDeepSnore,
  kDream,
  kCount
};

// What plays a pastime's body: a rig clip, an engine bit or an engine take.
enum class Body : uint8_t { kClip, kBit, kTake };
// A pastime: its body and that body's name, the look it's for, the tier it
// needs (1 to 5, 0 for none), the wait, in seconds of its look, the tags
// any one of which it needs (`party|newyear`, or none), and whether it's a
// move.
struct PastimeInfo {
  Pastime id;
  Body body;
  const char* name;
  SceneState look;
  uint8_t tier;
  uint16_t afterS;
  const char* tag = nullptr;
  bool move = false;
};
constexpr int kPastimeCount = int(Pastime::kCount) - 1;
extern const PastimeInfo kPastimes[kPastimeCount];
const PastimeInfo& info(Pastime p);
// A row's body in the pack: its rig clip, its bit (an index of
// slime::kGags) or its take (of slime::kPastimeTakes); none (nullptr or -1)
// for one of the other bodies, or one the pack hasn't got.
const rig::Clip* clipOf(const PastimeInfo& p);
int bitOf(const PastimeInfo& p);
int takeOf(const PastimeInfo& p);
// Whether the pack has a row's body, and the tag bits it needs any one of:
// each found once, by name, and kept.
bool hasBody(const PastimeInfo& p);
uint32_t tagsOf(const PastimeInfo& p);
// Whether a row fits now: its tier, wait and tag by the tags on and how
// long the look has shown.
bool fits(const PastimeInfo& p, uint32_t when, uint32_t lookMs);

// The replies to what you said (FaceContext::reply) that are a pastime's
// own: asked for while the look is idle or asleep, the face plays the
// pastime itself, at once, whatever its tier; at work, the reply's clip
// alone (rig/clips/reply-<name>.json). A nap is the snuggle under its
// blanket, a dance the dance.
struct ReplyPastime {
  const char* reply;
  Pastime pastime;
};
constexpr ReplyPastime kReplyPastimes[] = {{"nap", Pastime::kSnuggle}, {"dance", Pastime::kDance}};
// The pastime a reply asks for, kNone for one that asks for none.
Pastime pastimeFor(const char* reply);

// The rest between two, in ms of the device's clock, and how often the
// newest tier's are dealt while an older one's fit too, in percent.
constexpr uint32_t kRestMinMs = 20000, kRestMaxMs = 45000;
constexpr int kNewestPct = 60;
// A move starts at least this long after the last one did.
constexpr uint32_t kMoveGapMs = 300000;
// Putting the headphones on: they come down from kPhonesDropPx above its
// ears, held by a hand at each, and are on kPhonesOnMs in. While it melts
// they're off its head (it has none to hold them): gone over kMeltOffMs
// from kMeltOffAt, back over kMeltOnMs by kMeltOnBy.
constexpr int kPhonesDropPx = 120;
constexpr uint32_t kPhonesOnMs = 1300;
constexpr int kMeltOffAt = 600, kMeltOffMs = 600, kMeltOnBy = 5400, kMeltOnMs = 500;

// The tier `when` has reached (render/tags.h's tier1 to tier5), 0 for none.
int tierOf(uint32_t when);
// The tag bit of tier `n` (1 to 5).
uint32_t tierBit(int n);

// What the face shows of them at the device's time `t`, with the tags on:
// the pastime playing (or a look's occasion, with no drawing of its own)
// and how far in, at `fade` (0 … 255), and the kit, the headphones, at
// their opacity (0 … 255).
struct PastimeShow {
  Pastime playing = Pastime::kNone;
  uint32_t age = 0, ms = 1;
  int fade = 0;
  uint32_t t = 0, when = 0;
  int headphones = 0;
  // How far above its ears the headphones are, in px: coming down while
  // they're put on.
  int drop = 0;
};
// Where a pastime puts the desk's paws, each (0 left, 1 right) in the
// thousandths of the surface paintProps draws them in: lifted, moved out
// and at an opacity (0 … 255). At rest, typing as they do.
struct Paws {
  int lift[2] = {0, 0}, dx[2] = {0, 0}, alpha[2] = {255, 255};
  int melted = 0;  // how far it's melted, Q12 (PaintControls::melt): they fade with it, and the keyboard
};
Paws pawsFor(const PastimeShow& show);

// The kit it wears, the headphones, after what it wears
// for the tags (render/gel/layers.h) and before the cards.
void paintWorn(Surface& surface, const PastimeShow& show, const rig::Anchors& at);
// In front: what the pastime playing holds, throws, blows or dreams, at
// the body's anchors.
void paintPastimes(Surface& surface, const PastimeShow& show, const rig::Anchors& at, const Engine& e);

}  // namespace render::gel
