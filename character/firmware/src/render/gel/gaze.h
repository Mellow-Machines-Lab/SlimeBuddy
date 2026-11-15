// Where the gel looks by itself: THE SCREEN'S RULE, part three (parts one
// and two are render/gel/layers.h and pastimes.h). Its eyes rest forward,
// and turn to one thing at a time, for a reason you can name, held long
// enough to read and never darting:
//
// 1. Something needs you: it looks up at you for kNoticeMs, then turns to
//    the Mac, on the side the Mac says it's on (the `macleft` or `macright`
//    tag), and stays turned while it waits. With no side told, it rests.
// 2. Typing along with you (render/gel/face.h), while its paws tap: down
//    at the keys.
// 3. At work: a glance at a word as it starts to fall, followed down for
//    kGlanceMs, then forward again for at least kRestMs before the next;
//    kWatchRestMs while you're at the Mac with your hands still (the
//    `reading` tag), when it watches the work with you.
//
// The first that applies wins, and none applies while anything else has
// the eyes: a finger, a Moment, a pastime, a transition, a word forward, a
// work act, sleep. A new kind of look is one more rule here and nothing
// anywhere else: the face gathers the facts (GelFace::showGaze) and the
// engine turns the eyes (Engine::attend), slower than they follow a finger.
#pragma once
#include <cstdint>

namespace render::gel {

// What there is to look at now.
struct GazeFacts {
  bool free = false;      // the look shows, and nothing else has the eyes
  bool needsYou = false;  // the needs-you look, shown for lookMs
  uint32_t lookMs = 0;
  int mac = 0;            // the Mac's side: -1 left, 1 right, 0 not told
  bool along = false;     // typing along with you, its paws tapping
  bool watching = false;  // at work, and you're there with your hands still
  bool word = false;      // the newest word falling: when it dropped, where
  uint32_t wordBorn = 0;  // it is now, px of the 502 by 410 screen, and how
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
  // Where the points are: up at you, off to the Mac's side at the eyes'
  // height, and down at the keys.
  static constexpr int kYouY = -160, kMacReach = 420, kEyesY = 205, kKeysY = 400, kMidX = 251;
  GazePoint look(const GazeFacts& f, uint32_t t);
  void reset() { *this = Gaze{}; }

 private:
  // The word it glanced at: it follows that one down, whatever drops after.
  bool glancing_ = false, rested_ = true;
  uint32_t glanceAt_ = 0, glanceBorn_ = 0, restFrom_ = 0;
  int glanceX_ = 0, glanceY_ = 0, glanceSpeed_ = 0;
};

}  // namespace render::gel
