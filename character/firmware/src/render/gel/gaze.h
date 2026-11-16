// Where the gel looks by itself: THE SCREEN'S RULE, part three (parts one
// and two are render/gel/layers.h and pastimes.h). Its eyes rest forward,
// and turn to one thing at a time, for a reason you can name, held long
// enough to read and never darting:
//
// 1. Something needs you: it looks up at you for kNoticeMs, then turns to
//    the Mac, on the side the Mac says it's on (the `macleft` or `macright`
//    tag), and stays turned while it waits. With no side told, it rests.
// 2. Typing along with you (render/gel/face.h), idle or at work, while its
//    paws tap: down at the keys, so no glances then: heads down, as you are.
// 3. Thinking with you: a stretch of typing along of kThinkAfterMs or more
//    has just stopped, and it looks up at you for kThinkMs, then rests.
// 4. At work in an activity's own pose, looking, running or waiting: rest.
//    The pose's clip moves the eyes itself (the rig's gazeX and gazeY: down
//    a page, fixed on the monitor, out at you), and a glance on top would
//    pull against it. One it was in the middle of is dropped.
// 5. On a call (the `call` tag), with you at the Mac (GelFace::showWithYou):
//    rest. It keeps still.
// 6. The pointer: while your pointer moves and the Mac knows where the board
//    sits (`state.ptr`), its eyes follow it, as you see the board beside the
//    screen: further the further it is, up to kPointerReach. At the edge of
//    the screen nearest the board, it looks right at it, to the Mac's side.
//    Let go once the Mac stops sending it (still 3 s): rest.
// 7. Reading along while you scroll (the `scrolling` tag, showWithYou):
//    turned to the Mac's side (forward with no side told), its eyes drift
//    down a line at a time while you scroll, kLineMs a line, and flick back
//    up to the top as you stop; on until the read's linger is over.
// 8. Watching a video with you (the `watching` tag): turned to the Mac's
//    side at the eyes' height, and still.
// 9. At work, typing: a glance at a word as it starts to fall, followed
//    down for kGlanceMs, then forward again for at least kRestMs before the
//    next; kWatchRestMs while you're at the Mac with your hands still (the
//    `reading` tag), when it watches the work with you.
//
// The first that applies wins, and none applies while anything else has
// the eyes: a finger, a Moment, a pastime, a transition, a word forward,
// sleep. A new kind of look is one more rule here and nothing anywhere
// else: the face gathers the facts (GelFace::showGaze) and the engine turns
// the eyes (Engine::attend), slower than they follow a finger.
#pragma once
#include <cstdint>

namespace render::gel {

// What it does with you at the Mac besides typing along, the one the face
// shows (GelFace::showWithYou), lowest first.
enum class WithYou : uint8_t { kNone, kMusic, kWatching, kReading, kHolding, kCall };

// What there is to look at now.
struct GazeFacts {
  bool free = false;      // the look shows, and nothing else has the eyes
  bool needsYou = false;  // the needs-you look, shown for lookMs
  uint32_t lookMs = 0;
  int mac = 0;            // the Mac's side: -1 left, 1 right, 0 not told
  bool along = false;     // typing along with you, its paws tapping
  bool thinking = false;  // a long stretch of typing along stopped, thinkMs ago
  uint32_t thinkMs = 0;
  bool posed = false;                         // at work in a pose whose clip has the eyes: looking, running, waiting
  WithYou with = WithYou::kNone;              // what it does with you besides
  bool scrolling = false;                     // reading along, and you scroll now
  bool pointer = false, pointerNear = false;  // the way to your pointer (state.ptr)
  int pointerX = 0, pointerY = 0;
  bool watching = false;                    // at work, and you're there with your hands still
  bool word = false;                        // the newest word falling: when it dropped, where
  uint32_t wordBorn = 0;                    // it is now, px of the 502 by 410 screen, and how
  int wordX = 0, wordY = 0, wordSpeed = 0;  // fast it falls, px a second
};

// A point to look at, in the screen's px (it may be off the screen), or
// nothing: rest.
struct GazePoint {
  bool on = false;
  int x = 0, y = 0;
};

class Gaze {
 public:
  static constexpr uint32_t kNoticeMs = 1200, kGlanceMs = 1600, kRestMs = 7000, kWatchRestMs = 3000;
  // A word is caught only as it starts to fall: one already this old isn't
  // glanced at.
  static constexpr uint32_t kCatchMs = 500;
  // Thinking with you: after typing along this long, it looks up this long.
  static constexpr uint32_t kThinkAfterMs = 8000, kThinkMs = 2000;
  // Reading along: a line's drift down, how far it goes, in screen px, and
  // how many lines before it's at the foot of the page and starts again.
  static constexpr uint32_t kLineMs = 1400;
  static constexpr int kLinePx = 30, kLines = 4;
  // Where the points are: up at you, off to the Mac's side at the eyes'
  // height, and down at the keys; the reading's page to the Mac's side,
  // from its top; how far across and up or down the pointer at its
  // furthest (state.ptr's 10) takes the eyes.
  static constexpr int kYouY = -160, kMacReach = 420, kEyesY = 205, kKeysY = 400, kMidX = 251;
  static constexpr int kPageReach = 300, kPageTopY = 235;
  static constexpr int kPointerReach = 10, kPointerX = 42, kPointerY = 36;
  GazePoint look(const GazeFacts& f, uint32_t t);
  void reset() { *this = Gaze{}; }

 private:
  // The word it glanced at: it follows that one down, whatever drops after.
  bool glancing_ = false, rested_ = true;
  uint32_t glanceAt_ = 0, glanceBorn_ = 0, restFrom_ = 0;
  int glanceX_ = 0, glanceY_ = 0, glanceSpeed_ = 0;
  // Reading along: since when the eyes are going down the page, while you
  // scroll; none at its top.
  bool reading_ = false;
  uint32_t lineFrom_ = 0;
  bool wasScrolling_ = false;
};

}  // namespace render::gel
