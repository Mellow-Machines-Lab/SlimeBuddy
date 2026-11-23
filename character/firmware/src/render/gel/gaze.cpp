#include "render/gel/gaze.h"

namespace render::gel {

GazePoint Gaze::look(const GazeFacts& f, uint32_t t) {
  if (!f.free) {
    glancing_ = false;
    return {};
  }
  if (f.needsYou) {
    glancing_ = false;
    if (f.lookMs < kNoticeMs) return {true, kMidX, kYouY};
    if (f.mac) return {true, kMidX + f.mac * kMacReach, kEyesY};
    if (f.up) return {true, kMidX, kScreenUpY};
    return {};
  }
  if (f.along) {
    glancing_ = false;
    return {true, kMidX, kKeysY};
  }
  if (f.thinking && f.thinkMs < kThinkMs) {
    glancing_ = false;
    return {true, kMidX, kYouY};
  }
  if (f.posed || f.with == WithYou::kCall) {
    glancing_ = false;
    return {};
  }
  if (f.pointer) {
    glancing_ = false;
    if (f.pointerNear) {
      // Right at it, to the Mac's side: where the pointer points from.
      const int side = f.pointerX > 0 ? 1 : f.pointerX < 0 ? -1 : f.mac;
      if (side) return {true, kMidX + side * kMacReach, kEyesY};
      if (f.up) return {true, kMidX, kScreenUpY};
    }
    return {true, kMidX + f.pointerX * kPointerX, kEyesY + f.pointerY * kPointerY};
  }
  if (f.with == WithYou::kReading) {
    glancing_ = false;
    return {true, kMidX + f.mac * kPageReach, f.up ? kPageUpY : kPageY};
  }
  if (f.with == WithYou::kVideo) {
    glancing_ = false;
    return {true, kMidX + f.mac * kPageReach, f.up ? kYouY : kEyesY};
  }
  if (glancing_) {
    if (t - glanceAt_ < kGlanceMs)
      return {true, glanceX_, glanceY_ + int(int64_t(t - glanceAt_) * glanceSpeed_ / 1000)};
    glancing_ = false, rested_ = false, restFrom_ = t;
  }
  if (!rested_ && t - restFrom_ >= (f.watching ? kWatchRestMs : kRestMs)) rested_ = true;
  if (rested_ && f.word && f.wordBorn != glanceBorn_ && t - f.wordBorn < kCatchMs) {
    glancing_ = true, glanceAt_ = t, glanceBorn_ = f.wordBorn;
    glanceX_ = f.wordX, glanceY_ = f.wordY, glanceSpeed_ = f.wordSpeed;
    return {true, f.wordX, f.wordY};
  }
  return {};
}

}  // namespace render::gel
