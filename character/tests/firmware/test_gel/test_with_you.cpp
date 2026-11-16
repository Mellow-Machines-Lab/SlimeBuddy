// With you at the Mac besides typing along (render/gel/face.h, showWithYou)
// and where the eyes go for it (render/gel/gaze.h): the pointer, reading
// along, watching, music, a call, a copy held and typing fast; each rule's
// place against the others and against what has the body, and nothing at
// all while the Mac sends none of it.
#include <unity.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "app/behaviour.h"
#include "render/gel/face.h"
#include "render/gel/gaze.h"
#include "render/gel/painter.h"
#include "render/gel/pastimes.h"
#include "render/tags.h"

using namespace render;
using namespace render::gel;

namespace {

struct Board {
  linkkit::Rng rng;
  app::Behaviour b{std::make_unique<GelFace>()};
  app::Model m;
  uint32_t t = 0;
  Board(SceneState look, uint32_t when) {
    b.reset(0, rng);
    m.mood = Mood::kCalm;
    m.base = look;
    m.act = SceneState::kToolUse;  // typing: no activity's pose has the body
    m.when = reached(when);
    b.onState(m, 0);
  }
  GelFace& face() { return static_cast<GelFace&>(b.face()); }
  void state() { b.onState(m, t); }
  void tags(uint32_t when) {
    m.when = reached(when);
    state();
  }
  void pointer(bool on, int x = 0, int y = 0, bool near = false) {
    m.pointer.on = on, m.pointer.x = int8_t(x), m.pointer.y = int8_t(y), m.pointer.near = near;
    state();
  }
  template <typename Each>
  void to(uint32_t until, Each each) {
    for (; t + 16 <= until;) {
      t += 16;
      if (t % 10000 < 16) state();
      b.advance(t, rng);
      each();
    }
  }
  void to(uint32_t until) {
    to(until, [] {});
  }
  int channel(rig::Channel c) { return face().channels()[c]; }
  const char* cueing() { return face().cueing() ? face().cueing()->id : ""; }
};

const uint32_t kDay = tagBit("day");

}  // namespace

// Pins the rule list's order (gaze.h): needs you, typing along, thinking
// with you, a pose's clip, a call, the pointer, reading along, watching,
// the work glance; each only while the eyes are free.
void test_the_eyes_with_you_in_order() {
  static_assert(Gaze::kThinkAfterMs == 8000 && Gaze::kThinkMs == 2000, "thinking with you, as this test pins");
  static_assert(Gaze::kLineMs == 1400 && Gaze::kLinePx == 30 && Gaze::kLines == 4, "a line's drift, as this test pins");
  Gaze g;
  GazeFacts f;
  f.free = true;
  TEST_ASSERT_FALSE(g.look(f, 0).on);  // nothing with you: rest
  // The pointer: its way, as you see the board.
  f.pointer = true, f.pointerX = -10, f.pointerY = 0;
  GazePoint p = g.look(f, 16);
  TEST_ASSERT_TRUE(p.on && p.x < Gaze::kMidX - 300 && p.y == Gaze::kEyesY);
  f.pointerX = 5, f.pointerY = -10;
  p = g.look(f, 32);
  TEST_ASSERT_TRUE(p.x > Gaze::kMidX && p.y < Gaze::kYouY + 20);  // up and to its right
  f.pointerX = 1, f.pointerY = 0, f.pointerNear = true;
  p = g.look(f, 48);
  TEST_ASSERT_TRUE(p.x == Gaze::kMidX + Gaze::kMacReach && p.y == Gaze::kEyesY);  // right at it, at the edge
  f.pointerNear = false;
  // Above it: a call keeps the eyes still; a pose's clip has them; thinking
  // looks up at you; typing along is down at the keys; needs you first.
  f.with = WithYou::kCall;
  TEST_ASSERT_FALSE(g.look(f, 64).on);
  f.with = WithYou::kNone, f.posed = true;
  TEST_ASSERT_FALSE(g.look(f, 80).on);
  f.thinking = true, f.thinkMs = 100;
  TEST_ASSERT_TRUE(g.look(f, 96).y == Gaze::kYouY);
  f.thinkMs = Gaze::kThinkMs;
  TEST_ASSERT_FALSE(g.look(f, 112).on);  // its moment is over
  f.along = true;
  TEST_ASSERT_TRUE(g.look(f, 128).y == Gaze::kKeysY);
  f.needsYou = true, f.lookMs = 0;
  TEST_ASSERT_TRUE(g.look(f, 144).y == Gaze::kYouY);
  // Below the pointer: reading along, then watching, then the glance.
  Gaze r;
  f = GazeFacts{};
  f.free = true, f.mac = 1, f.with = WithYou::kReading, f.scrolling = true;
  f.word = true, f.wordBorn = 1000, f.wordX = 84;
  p = r.look(f, 1000);
  TEST_ASSERT_TRUE(p.on && p.x == Gaze::kMidX + Gaze::kPageReach && p.y == Gaze::kPageTopY);          // the page's top
  TEST_ASSERT_EQUAL_INT(Gaze::kPageTopY + Gaze::kLinePx / 2, r.look(f, 1000 + Gaze::kLineMs / 2).y);  // down a line
  TEST_ASSERT_EQUAL_INT(Gaze::kPageTopY + Gaze::kLinePx * 2, r.look(f, 1000 + 2 * Gaze::kLineMs).y);
  TEST_ASSERT_EQUAL_INT(Gaze::kPageTopY, r.look(f, 1000 + Gaze::kLines * Gaze::kLineMs).y);  // the next page
  f.scrolling = false;
  TEST_ASSERT_EQUAL_INT(Gaze::kPageTopY, r.look(f, 7000).y);  // you stop: back up, and it reads on
  f.pointer = true, f.pointerX = -3;
  TEST_ASSERT_TRUE(r.look(f, 7016).x < Gaze::kMidX);  // the pointer comes first
  f.pointer = false, f.mac = 0;
  TEST_ASSERT_EQUAL_INT(Gaze::kMidX, r.look(f, 7032).x);  // no side told: forward
  f.with = WithYou::kWatching, f.mac = -1;
  p = r.look(f, 7048);
  TEST_ASSERT_TRUE(p.x == Gaze::kMidX - Gaze::kPageReach && p.y == Gaze::kEyesY);
  f.free = false;
  TEST_ASSERT_FALSE(r.look(f, 7064).on);  // something else has the eyes
}

// Pins the pointer: idle, the eyes follow it and the body leans a little its
// way, further at the edge nearest the board, reaching up toward it; let
// go, back to rest. Not in a work pose, not under needs you, nothing with
// no pointer.
void test_the_eyes_follow_your_pointer() {
  static_assert(GelFace::kLeanReach == 160 && GelFace::kPointerLean == 14 && GelFace::kPointerStretch == 6 &&
                    GelFace::kNearStretch == 50,
                "the lean this test pins");
  Board board(SceneState::kIdle, kDay | tierBit(1));
  GelFace& face = board.face();
  board.to(1000);
  TEST_ASSERT_FALSE(face.engine().motion.attending);
  TEST_ASSERT_EQUAL_INT(0, board.channel(rig::Channel::kLean) - face.mixer().channels()[rig::Channel::kLean]);
  board.pointer(true, -8, -4);
  board.to(board.t + 1200);
  TEST_ASSERT_TRUE(face.engine().motion.attending && face.engine().motion.attendX < F(0) &&
                   face.engine().motion.attendUp > F(0));
  const int lean = board.channel(rig::Channel::kLean) - face.mixer().channels()[rig::Channel::kLean];
  TEST_ASSERT_TRUE(lean < -100 && lean >= -112);  // its way: 14 a step, eased
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kSquash) - face.mixer().channels()[rig::Channel::kSquash] <
                   -20);  // up: taller
  board.pointer(true, 9, 0);
  board.to(board.t + 1200);
  TEST_ASSERT_TRUE(face.engine().motion.attendX > F(0));
  // At the edge nearest the board: right at it, leaning in, stretched toward it.
  board.pointer(true, 1, 0, true);
  board.to(board.t + 1200);
  const rig::Channels mixed = face.mixer().channels();
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kLean) - mixed[rig::Channel::kLean] > 150);
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kSquash) - mixed[rig::Channel::kSquash] < -45);
  // Moving the eyes and the body never pops: the lean eases.
  board.pointer(true, -10, 0);
  int last = board.channel(rig::Channel::kLean) - face.mixer().channels()[rig::Channel::kLean], jump = 0;
  board.to(board.t + 800, [&] {
    const int now = board.channel(rig::Channel::kLean) - face.mixer().channels()[rig::Channel::kLean];
    jump = std::max(jump, std::abs(now - last)), last = now;
  });
  TEST_ASSERT_TRUE(jump <= 20);  // at most 20 thousandths a frame
  // The Mac stops sending it: rest.
  board.pointer(false);
  board.to(board.t + 2000);
  TEST_ASSERT_FALSE(face.engine().motion.attending);
  TEST_ASSERT_TRUE(std::abs(board.channel(rig::Channel::kLean) - face.mixer().channels()[rig::Channel::kLean]) <= 1);
  // At work in a pose with the eyes of its own: nothing.
  Board work(SceneState::kWorking, kDay);
  work.m.act = SceneState::kAnalyzing;
  work.pointer(true, -8, 0);
  work.to(3000, [&] {
    TEST_ASSERT_FALSE(work.face().engine().motion.attending && work.face().engine().motion.attendX < F(0));
  });
  // In its typing pose, yes.
  work.m.act = SceneState::kToolUse;
  work.state();
  work.to(work.t + 1200);
  TEST_ASSERT_TRUE(work.face().engine().motion.attending && work.face().engine().motion.attendX < F(0));
  // Needs you comes first.
  work.m.attn = true, work.m.attnId = 3;
  std::strcpy(work.m.agent, "claude");
  work.state();
  work.to(work.t + 600);
  TEST_ASSERT_TRUE(work.face().engine().motion.attendUp > F(0) && abs(work.face().engine().motion.attendX) < q(.01));
}

// Pins reading along: while you scroll, and kReadLingerMs after, it reads
// along, turned to the Mac's side and a little lower, no pastime dealt; a
// read of kSettleMs settles it in, its lids half down; and none at work in
// a pose of its own.
void test_it_reads_along_while_you_scroll() {
  static_assert(GelFace::kReadLingerMs == 6000 && GelFace::kSettleMs == 120000 && GelFace::kReadGapMs == 60000,
                "the times this test pins");
  const uint32_t scrolling = tagBit("scrolling"), right = tagBit("macright");
  Board board(SceneState::kIdle, kDay | tierBit(2) | right);
  GelFace& face = board.face();
  board.to(1000);
  board.tags(kDay | tierBit(2) | right | scrolling);
  board.to(board.t + 1000);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kReading), int(face.withYou()));
  TEST_ASSERT_EQUAL_STRING("reading", face.passing(board.t));
  TEST_ASSERT_TRUE(face.engine().motion.attending && face.engine().motion.attendX > F(0));  // to the Mac, on its right
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kLean) > 120 && board.channel(rig::Channel::kSquash) > 20);
  // You stop: it reads on a while, then rests.
  board.tags(kDay | tierBit(2) | right);
  const uint32_t stopped = board.t;
  board.to(stopped + GelFace::kReadLingerMs - 64);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kReading), int(face.withYou()));
  board.to(stopped + GelFace::kReadLingerMs + 64);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kNone), int(face.withYou()));
  // Two minutes of reading along, scrolling now and then: settled in, lids
  // half down, and never a pastime over it.
  bool pastime = false;
  int lids = 1000;
  for (int i = 0; i < 30; ++i) {
    board.tags(kDay | tierBit(2) | right | scrolling);
    board.to(board.t + 2000, [&] { pastime = pastime || face.pastimes(board.t).playing != Pastime::kNone; });
    board.tags(kDay | tierBit(2) | right);
    board.to(board.t + 3000, [&] { pastime = pastime || face.pastimes(board.t).playing != Pastime::kNone; });
  }
  board.to(board.t + 800, [&] { lids = std::min(lids, int(board.channel(rig::Channel::kEyeOpenL))); });
  TEST_ASSERT_FALSE(pastime);
  TEST_ASSERT_TRUE(lids < 600);  // settled in for a comfortable read
  // At work in a pose of its own: no reading along.
  Board work(SceneState::kWorking, kDay | right | scrolling);
  work.m.act = SceneState::kTerminal;
  work.state();
  work.to(3000, [&] { TEST_ASSERT_EQUAL_INT(int(WithYou::kNone), int(work.face().withYou())); });
}

// Pins music: it gets into it kGrooveMs (eyes half closed, a slow sway and
// a nod, no hop), then rests kGrooveRestMs, and again; as the music stops
// it looks round once. Watching a video: turned to the screen, still.
void test_it_grooves_to_your_music_and_watches_with_you() {
  static_assert(GelFace::kGrooveMs == 24000 && GelFace::kGrooveRestMs == 180000, "the groove this test pins");
  const uint32_t music = tagBit("music"), left = tagBit("macleft");
  Board board(SceneState::kIdle, kDay | tierBit(1) | left);
  GelFace& face = board.face();
  board.to(1000);
  board.tags(kDay | tierBit(1) | left | music);
  const uint32_t from = board.t;
  int low = 1 << 30, high = -(1 << 30), lifted = 0, lids = 1000;
  board.to(from + 12000, [&] {
    if (board.t < from + 1000) return;
    const int lean = board.channel(rig::Channel::kLean);
    low = std::min(low, lean), high = std::max(high, lean),
    lifted = std::max(lifted, board.channel(rig::Channel::kLift));
    lids = std::min(lids, board.channel(rig::Channel::kEyeOpenL));
  });
  TEST_ASSERT_EQUAL_INT(int(WithYou::kMusic), int(face.withYou()));
  TEST_ASSERT_TRUE(high - low > 100 && high - low < 200);  // a slow sway, side to side
  TEST_ASSERT_EQUAL_INT(0, lifted);                        // never a hop
  TEST_ASSERT_TRUE(lids < 650);                            // eyes half closed
  board.to(from + GelFace::kGrooveMs + 100);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kNone), int(face.withYou()));  // it rests
  board.to(from + GelFace::kGrooveMs + GelFace::kGrooveRestMs + 100);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kMusic), int(face.withYou()));  // and again
  board.tags(kDay | tierBit(1) | left);
  board.to(board.t + 64);
  TEST_ASSERT_EQUAL_STRING("look-around", board.cueing());  // the music's stopped: a look round
  // Watching: to the screen, on its left, its eyes still.
  Board video(SceneState::kIdle, kDay | tierBit(1) | left | tagBit("watching"));
  video.to(2000);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kWatching), int(video.face().withYou()));
  TEST_ASSERT_TRUE(video.face().engine().motion.attendX < F(0) && video.channel(rig::Channel::kLean) < -120);
}

// Pins the call: still and a little smaller, its eyes half closed; no
// typing along, no music, no pastime however long; a stretch as it ends.
// It comes before a copy held, a read and music.
void test_a_call_keeps_it_still() {
  const uint32_t call = tagBit("call"), typing = tagBit("typing"), music = tagBit("music");
  Board board(SceneState::kIdle, kDay | tierBit(3));
  GelFace& face = board.face();
  board.to(1000);
  board.tags(kDay | tierBit(3) | call | typing | music | tagBit("holding") | tagBit("scrolling"));
  bool moved = false;
  board.to(board.t + 300000, [&] {
    moved = moved || face.pastimes(board.t).tapping || face.pastimes(board.t).playing != Pastime::kNone;
  });
  TEST_ASSERT_FALSE(moved);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kCall), int(face.withYou()));
  TEST_ASSERT_EQUAL_STRING("call", face.passing(board.t));
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kSquash) > 120 && board.channel(rig::Channel::kEyeOpenL) < 650);
  TEST_ASSERT_FALSE(face.engine().motion.attending);  // its eyes still, forward
  board.tags(kDay | tierBit(3));
  board.to(board.t + 64);
  TEST_ASSERT_EQUAL_STRING("stretch", board.cueing());
}

// Pins holding: a copy held puffs its cheeks; let go, a pah, once.
void test_it_holds_a_copy_and_lets_it_go() {
  const uint32_t holding = tagBit("holding");
  Board board(SceneState::kIdle, kDay | tierBit(1));
  GelFace& face = board.face();
  board.to(1000);
  const int rest = board.channel(rig::Channel::kSquash);
  board.tags(kDay | tierBit(1) | holding);
  board.to(board.t + 1000);
  TEST_ASSERT_EQUAL_INT(int(WithYou::kHolding), int(face.withYou()));
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kSquash) - rest > 50);  // puffed up
  board.tags(kDay | tierBit(1));
  board.to(board.t + 64);
  TEST_ASSERT_EQUAL_STRING("release", board.cueing());
  int open = 0;
  board.to(board.t + 400, [&] { open = std::max(open, board.channel(rig::Channel::kMouthOpen)); });
  TEST_ASSERT_TRUE(open >= kRigMouthOpen);  // pah
}

// Pins typing fast: its paws tap quicker, as high, and its eyes open a little
// wider than typing along's lowered lids.
void test_typing_fast_is_quicker_not_higher() {
  static_assert(kFastMs < kBriskMs && kFastTapMs < 2 * kFastMs, "fast is quicker, and its taps don't run together");
  TEST_ASSERT_EQUAL_INT(kBriskLift, typeLift(2 * kFastMs - kFastTapMs / 2, -1, true, true));
  const uint32_t typing = tagBit("typing"), fast = tagBit("typingfast");
  Board board(SceneState::kIdle, kDay | tierBit(1));
  GelFace& face = board.face();
  board.tags(kDay | tierBit(1) | typing);
  board.to(1500);
  const int lids = board.channel(rig::Channel::kEyeOpenL);
  TEST_ASSERT_FALSE(face.pastimes(board.t).fast);
  board.tags(kDay | tierBit(1) | typing | fast);
  board.to(board.t + 800);
  TEST_ASSERT_TRUE(face.pastimes(board.t).tapping && face.pastimes(board.t).fast);
  TEST_ASSERT_TRUE(board.channel(rig::Channel::kEyeOpenL) - lids > 100);
}

// Nothing with you while the Mac sends none of it; and every one gives way
// to needs you, a Moment's face, sleep and a work pose of its own.
void test_nothing_with_you_without_the_facts() {
  Board board(SceneState::kIdle, kDay | tierBit(2));
  GelFace& face = board.face();
  board.to(60000, [&] {
    TEST_ASSERT_EQUAL_INT(int(WithYou::kNone), int(face.withYou()));
    TEST_ASSERT_EQUAL_INT(face.mixer().channels()[rig::Channel::kLean], board.channel(rig::Channel::kLean));
  });
  const uint32_t all = tagBit("music") | tagBit("watching") | tagBit("scrolling") | tagBit("holding");
  for (SceneState look : {SceneState::kAsleep, SceneState::kNeedsYou}) {
    Board other(look == SceneState::kNeedsYou ? SceneState::kWorking : look, kDay | all);
    if (look == SceneState::kNeedsYou) {
      other.m.attn = true, other.m.attnId = 5;
      std::strcpy(other.m.agent, "codex");
      other.state();
    }
    other.to(5000, [&] { TEST_ASSERT_EQUAL_INT(int(WithYou::kNone), int(other.face().withYou())); });
  }
}
