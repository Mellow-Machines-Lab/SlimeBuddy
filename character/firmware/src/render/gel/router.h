#pragma once
#include <cstdint>
#include <string>
#include "render/types.h"
#include "render/gel/time.h"
namespace render::gel {
enum class Family { kRest, kIdle, kListen, kStart, kDesk, kHelpers, kAlert, kResult, kTouch };
Family family(SceneState state);
bool isDesk(SceneState state);
struct Attention {
  bool on = false;
  std::string id = "0", agent, project, name;
  int more = 0;
};
struct Facts {
  SceneState base = SceneState::kIdle, act = SceneState::kWorking;
  Attention attention;
  SceneState look() const { return base == SceneState::kWorking ? act : base; }
};
// A reaction, or listening, as the board's Behaviour plays it: Behaviour
// times it, and the engine holds it until it's cleared or another comes.
struct Request {
  SceneState anim = SceneState::kNone;
  int expression = -1, loops = 1, variant = -1;
  Outcome outcome = Outcome::kNone;
  StartCtx ctx = StartCtx::kNone;
};
// What a route shows, as one number: what it is, and which one (a look's
// state, a needs-you's or a reaction's count). A scene keeps its take while
// its key stays. A bit's contacts are keyed as a bit's (kBit).
enum class Shows : uint8_t { kBit, kLook, kLostLink, kListening, kNeedsYou, kReaction };
constexpr uint64_t showKey(Shows what, uint32_t n) { return uint64_t(what) << 32 | n; }
constexpr Shows shows(uint64_t key) { return Shows(key >> 32); }
struct Route {
  SceneState state = SceneState::kIdle, baseState = SceneState::kIdle;
  Family group = Family::kIdle;
  bool quiet = false;
  uint64_t key = showKey(Shows::kLook, uint32_t(SceneState::kIdle));
  const Attention* attention = nullptr;
  // The reaction held, under whatever shows, and when it started.
  const Request* reaction = nullptr;
  T reactionAt;
  int expression = -1, loops = 1;
  Outcome outcome = Outcome::kNone;
  StartCtx ctx = StartCtx::kNone;
};
// What the engine shows: the look, needs you, listening, the reaction it
// holds, the face it borrows, and whether the link is lost. Each needs-you
// and each reaction is numbered as it comes.
struct Router {
  Facts facts;
  bool timedOut = false, listening = false, reacting = false;
  Request reaction;
  T reactionAt;
  // The borrowed face, over whatever shows, or -1.
  int expression = -1;
  uint32_t alerts = 0, reactions = 0;
  void setFacts(const Facts& value);
  // Listening, or a reaction, from `time`: each ends the one before. None
  // while the link is lost, and no reaction while needs you shows.
  bool play(const Request& value, T time);
  void clear() {
    listening = reacting = false;
    expression = -1;
  }
  Route resolve() const;
};
}  // namespace render::gel
