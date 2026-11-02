#include "render/gel/face.h"
#include "render/gel/act.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "render/gel/profile.h"
#include "render/maths.h"
#include "render/palette.h"
#include "render/gel/layers.h"
#include "render/gel/sounds.h"
#include "rig_tables.h"  // the staged pack's (characters/charactergen.py)

namespace render::gel {

namespace {

// The engine's profile for a mood: calm for one it doesn't know.
int profileFor(Mood mood) {
  int p = expressionFromName(moodName(mood));
  return p < 0 ? slime::kCalm : p;
}

constexpr int kTakes = int(sizeof(slime::kRecipes[0][0]) / sizeof(slime::kRecipes[0][0][0]));

}  // namespace

const voice::EffectClips& GelFace::clips() const { return gel::clips(); }

const slime::Recipe& GelFace::recipe(Mood mood, SceneState state, int variant, Outcome outcome, StartCtx ctx) {
  if (state == SceneState::kTaskComplete && outcome != Outcome::kFailure && isFinishVariant(variant))
    return finishRecipe(finishOf(variant));
  int row = recipeState(state, outcome, ctx);
  // A finish timed without its outcome: its fourth take is a success's.
  if (state == SceneState::kTaskComplete && variant >= takes(row)) row = recipeState(state, Outcome::kSuccess, ctx);
  return take(profileFor(mood), row, std::max(0, std::min(takes(row) - 1, variant)));
}

void GelFace::reset(uint32_t t, linkkit::Rng&) {
  engine_ = std::make_unique<Engine>(kSeed);
  engine_->step(0);
  origin_ = lastTick_ = lookAt_ = t;
  engineMs_ = engineFrac_ = 0;
  pace_ = 100;
  lastBig_ = Finish::kLittle;
  source_ = {};
  replyAt_ = 0, replied_ = false;
  finger_ = false;  // the new engine has no drag: a finger still down grabs it afresh
  pendingCount_ = 0;
  revision_ = cueRevision_ = 0;
  mixer_.reset(0, 0, nullptr);
  layers_ = {};
  clip_ = cueing_ = base_ = nullptr;
  pastime_ = lastPastime_ = Pastime::kNone;
  pastimeAt_ = pastimeEnds_ = stoppedAt_ = restUntil_ = zoneSince_ = 0;
  inZone_ = phonesWanted_ = phonesOn_ = moved_ = false;
  movedAt_ = 0;
  pastimeLook_ = dealtLook_ = SceneState::kNone;
  stopped_ = false;
  dice_ = kSeed;
  mug_ = headphones_ = zone_ = plant_ = rig::Ramp{};
  grown_ = 1;
}

// Power-on only, right after reset: the same engine with the board's seed.
void GelFace::seed(uint32_t s) {
  engine_ = std::make_unique<Engine>(s);
  engine_->step(0);
}

// The engine steps on a 16 ms grid from the last reset.
bool GelFace::nextDue(uint32_t from, uint32_t to, uint32_t& at) {
  uint32_t next = origin_ + ((from - origin_) / 16 + 1) * 16;
  if (after(next, to)) return false;
  at = next;
  return true;
}

const void* GelFace::assets(size_t& bytes) const { return materialTexels(bytes); }
void GelFace::readAssetsFrom(const void* copy) { readMaterialFrom(copy); }

void GelFace::restartCues() {
  ++revision_;
  pendingCount_ = 0;
}

void GelFace::advanceTo(uint32_t t) {
  if (t == lastTick_) return;
  Measure measure(gel::profile.engine);
  const uint64_t scaled = uint64_t(t - lastTick_) * pace_ + engineFrac_;
  engineMs_ += uint32_t(scaled / 100);
  engineFrac_ = uint32_t(scaled % 100);
  // The cues the clip playing crosses sound with this step's contacts.
  if (cueing_) {
    const char* names[Engine::kClipCues];
    const int n = rig::crossed(*cueing_, cueAge_, cueAge_ + (t - lastTick_), names, Engine::kClipCues);
    for (int i = 0; i < n; ++i) engine_->contact(names[i]);
    cueAge_ += t - lastTick_;
    if (cueAge_ >= cueing_->ms) cueing_ = nullptr;
  }
  engine_->step(engineMs_);
  mixer_.advance(t - lastTick_);
  lastTick_ = t;
  passTime(t);
  showBase();
  showLayers(t);
  showKit(t);
  int kept = 0;
  for (int i = 0; i < pendingCount_; ++i)
    if (t - pendingAt_[i] <= kCueWaitMs) {
      pending_[kept] = pending_[i];
      pendingAt_[kept++] = pendingAt_[i];
    }
  pendingCount_ = kept;
  for (const auto& cue : engine_->cues) {
    int clip = clipFor(cue.recipe, cue.variant);
    if (clip < 0 || pendingCount_ >= kMaxCues) continue;
    pendingAt_[pendingCount_] = t;
    voice::FxEvent& e = pending_[pendingCount_++];
    e.atMs = 0;
    e.clip = uint8_t(clip);
    e.gain = uint8_t(clamp(cue.gain * 255, 0, 255).integer());
    e.pitch = uint16_t((cue.pitch * 1000).integer());
    e.duck = !cue.protectedCue;  // needs you's and a finish's stay up under a line
  }
}

void GelFace::showLayers(uint32_t t) {
  const Contents now = layerContents(*engine_);
  layers_.show(rig::Layer::kScene, now.scene, t);
  layers_.show(rig::Layer::kWorn, now.worn, t);
  layers_.show(rig::Layer::kHat, now.hat, t);
  layers_.show(rig::Layer::kAir, now.air, t);
  layers_.show(rig::Layer::kBeside, now.beside, t);
}

uint32_t GelFace::roll(uint32_t n) {
  dice_ ^= dice_ << 13, dice_ ^= dice_ >> 17, dice_ ^= dice_ << 5;  // xorshift32
  return n ? dice_ % n : 0;
}

void GelFace::endPastime(uint32_t t, bool early) {
  if (early && clip_) mixer_.stop(*clip_);
  // One stopped early fades out what it draws; a bit's or a take's draws
  // nothing, and the engine stops it itself as the look gives way.
  stopped_ = early && clip_;
  stoppedAt_ = t;
  if (!stopped_) pastime_ = Pastime::kNone;
  clip_ = nullptr;
  restUntil_ = t + kRestMinMs + roll(kRestMaxMs - kRestMinMs);
}

bool GelFace::showsLook(SceneState anim, SceneState& look) const {
  const Engine& e = *engine_;
  const bool needsYou = e.router.facts.attention.on;
  look = needsYou ? SceneState::kNeedsYou : e.router.facts.base;
  const Route& route = e.scenes.route;
  return anim == SceneState::kNone && !route.reaction &&
         route.state == (needsYou ? SceneState::kNeedsYou : e.router.facts.look());
}

void GelFace::showBase() {
  SceneState look;
  const bool shows = showsLook(source_.anim, look);
  const rig::Look* l = rig::kRig.look(look);
  const rig::Clip* clip = l && shows ? l->tiered(engine_->scenes.when) : nullptr;
  if (clip != base_) mixer_.setLook(base_ = clip);
}

void GelFace::passTime(uint32_t t) {
  if (stopped_ && !within(t, stoppedAt_, rig::kFadeMs)) stopped_ = false, pastime_ = Pastime::kNone;
  if (playing() && !after(pastimeEnds_, t)) endPastime(t, false);
  SceneState look;
  const bool shows = showsLook(source_.anim, look);
  if (playing() && (!shows || look != pastimeLook_)) endPastime(t, true);
  // A new look rests before its first.
  if (look != dealtLook_) dealtLook_ = look, restUntil_ = t + kRestMinMs + roll(kRestMaxMs - kRestMinMs);
  // A work act holds the screen, as a finger does: what's playing goes.
  const bool acting = stream_ && actTakesOver(stream_->actNow(t));
  engine_->scenes.clearStage = acting;
  if (acting && playing()) endPastime(t, true);
  const bool busy = finger_ || acting || (stream_ && stream_->drawn(t) && stream_->call.live);
  if (!shows || pastime_ != Pastime::kNone || busy || after(restUntil_, t)) return;
  deal(look, t);
}

void GelFace::deal(SceneState look, uint32_t t) {
  const Engine& e = *engine_;
  const uint32_t when = e.scenes.when, lookMs = e.scenes.lookMs;
  struct Option {
    const PastimeInfo* row;
    int rank;  // the newest is the highest
  } options[kPastimeCount];
  int n = 0;
  // In the morning its coffee is in its hand (render/gel/layers.h): no mug
  // on the desk, so no sip, reach or refill from one. A move waits its gap.
  const bool morning = hasTag(when, "morning"), moveWaits = moved_ && within(t, movedAt_, kMoveGapMs);
  // Nor a melt in a hat, which has no head to sit on then.
  const bool hatted = outfitFor(when) != Outfit::kNone;
  // A take at work only while it types.
  const SceneState desk = e.scenes.route.state;
  const bool types = desk == SceneState::kWorking || desk == SceneState::kTerminal || desk == SceneState::kToolUse;
  for (const auto& p : kPastimes) {
    if (p.look != look || !fits(p, when, lookMs) || (p.move && moveWaits) || !hasBody(p)) continue;
    if (morning && (p.id == Pastime::kSip || p.id == Pastime::kReach || p.id == Pastime::kRefill)) continue;
    if ((hatted && p.id == Pastime::kMelt) || (p.body == Body::kTake && look == SceneState::kWorking && !types))
      continue;
    // The highest tier, then the longest wait, is the newest; one that
    // needs a tag is newer than any while it fits.
    options[n++] = {&p, (p.tag ? 1000000 : p.tier * 100000) + p.afterS};
  }
  // The headphones wanted and not yet on: they go on first, and only then.
  if (look == SceneState::kWorking && phonesWanted_ && !phonesOn_) {
    playPastime(Pastime::kPhonesOn, t);
    return;
  }
  for (int i = 0; i < n; ++i)
    if (options[i].row->id == Pastime::kPhonesOn) options[i--] = options[--n];
  // Never the one dealt last, while there's another.
  if (n > 1)
    for (int i = 0; i < n; ++i)
      if (options[i].row->id == lastPastime_) {
        options[i] = options[--n];
        break;
      }
  if (!n) return;
  int top = 0, newest = 0;
  for (int i = 0; i < n; ++i) top = std::max(top, options[i].rank);
  for (int i = 0; i < n; ++i) newest += options[i].rank == top;
  // The newest kNewestPct of the time, an older one the rest.
  const bool fresh = newest == n || int(roll(100)) < kNewestPct;
  int pick = int(roll(uint32_t(fresh ? newest : n - newest)));
  const Option* chosen = nullptr;
  for (int i = 0; i < n && !chosen; ++i)
    if ((options[i].rank == top) == fresh && pick-- == 0) chosen = &options[i];
  start(*chosen->row, look, t);
}

void GelFace::start(const PastimeInfo& p, SceneState look, uint32_t t) {
  if (playing()) endPastime(t, true);
  pastime_ = lastPastime_ = p.id;
  clip_ = nullptr;
  if (p.move) movedAt_ = t, moved_ = true;
  stopped_ = false;
  pastimeLook_ = dealtLook_ = look;
  pastimeAt_ = t;
  uint32_t ms = 1;
  if (const rig::Clip* clip = clipOf(p)) {
    clip_ = clip;
    ms = clip->ms;
    play(*clip);
    holdGestures(*clip);
  } else if (const int gag = bitOf(p); gag >= 0) {
    engine_->playBit(gag);
    ms = uint32_t(F(slime::kGags[gag].gesture.duration * 1000).integer());
  } else if (const int take = takeOf(p); take >= 0) {
    engine_->playTake(take);
    // The engine's time runs at the look's pace.
    ms = uint32_t(F(slime::kPastimeTakes[take].recipe.duration).integer()) * 100 / uint32_t(pace_);
  }
  pastimeEnds_ = t + std::max<uint32_t>(1, ms);
}

void GelFace::holdGestures(const rig::Clip& clip) {
  auto& acting = engine_->acting;
  acting.nextPerformance = max(acting.nextPerformance, engine_->motion.time + int(clip.ms / 1000) + 2);
}

void GelFace::playPastime(Pastime p, uint32_t t) {
  const PastimeInfo& i = info(p);
  if (hasBody(i)) start(i, i.look, t);
}

void GelFace::reply(const char* name, SceneState anim, uint32_t t) {
  SceneState look;
  const Pastime p = pastimeFor(name);
  if (p != Pastime::kNone && hasBody(info(p)) && showsLook(anim, look) &&
      (look == SceneState::kIdle || look == SceneState::kAsleep)) {
    start(info(p), look, t);
    return;
  }
  char clip[24];
  std::snprintf(clip, sizeof(clip), "reply-%s", name);
  if (const rig::Clip* c = rig::kRig.clip(clip)) play(*c);
}

bool GelFace::playPastime(const char* name, uint32_t t) {
  for (const PastimeInfo& p : kPastimes)
    if (!std::strcmp(p.name, name)) {
      advanceTo(t);
      playPastime(p.id, t);
      return true;
    }
  return false;
}

void GelFace::showKit(uint32_t t) {
  const Engine& e = *engine_;
  const auto& facts = e.router.facts;
  const SceneState base = facts.attention.on ? SceneState::kNeedsYou : facts.base;
  const uint32_t when = e.scenes.when;
  const int tier = tierOf(when);
  const bool working = base == SceneState::kWorking, idle = base == SceneState::kIdle;
  // The hat wins the head; and peekaboo's gesture takes it all off, as it
  // does the hat.
  const bool head = outfitFor(when) == Outfit::kNone &&
                    !(e.scenes.route.state == SceneState::kGesture && e.scenes.variant == int(Gesture::kPeekaboo));
  auto show = [t](rig::Ramp& ramp, bool on) {
    if (ramp.target() != (on ? 1000 : 0)) ramp.retarget(on ? 1000 : 0, t);
  };
  if (working && tier >= 5 && !inZone_) zoneSince_ = t;
  inZone_ = working && tier >= 5;
  show(zone_, inZone_);
  show(mug_, working && tier >= 1 && !hasTag(when, "morning"));  // the morning's is in its hand
  phonesWanted_ = working && tier >= 3 && head;
  if (!phonesWanted_) phonesOn_ = false;
  if (pastime_ == Pastime::kPhonesOn && !stopped_ && t - pastimeAt_ >= kPhonesOnMs) phonesOn_ = true;
  show(headphones_, phonesWanted_ && phonesOn_);
  show(plant_, idle && tier >= 3);
  if (idle && tier >= 3) grown_ = std::min(3, tier - 2);
}

PastimeShow GelFace::pastimes(uint32_t t) const {
  PastimeShow s;
  s.t = t, s.when = engine_->scenes.when;
  if (pastime_ != Pastime::kNone) {
    s.playing = pastime_;
    s.age = t - pastimeAt_;
    s.ms = pastimeEnds_ - pastimeAt_;
    // In over 150 ms, out over its last 250; one stopped early, out over
    // the rig's fade from then.
    int fade = int(std::min<uint32_t>({255, s.age * 255 / 150, s.age < s.ms ? (s.ms - s.age) * 255 / 250 : 0}));
    if (stopped_) fade = fade * int(rig::kFadeMs - std::min(rig::kFadeMs, t - stoppedAt_)) / int(rig::kFadeMs);
    s.fade = std::max(0, fade);
  }
  auto opacity = [t](const rig::Ramp& ramp) { return (ramp.value(t) * 255 + 500) / 1000; };
  s.mug = opacity(mug_);
  s.headphones = opacity(headphones_);
  if (pastime_ == Pastime::kMelt) {  // off its head while it's a puddle
    const int age = int(s.age), off = age < kMeltOffAt              ? 0
                                      : age < kMeltOffAt + kMeltOffMs ? (age - kMeltOffAt) * 255 / kMeltOffMs
                                      : age < kMeltOnBy - kMeltOnMs   ? 255
                                      : age < kMeltOnBy               ? (kMeltOnBy - age) * 255 / kMeltOnMs
                                                                      : 0;
    s.headphones = s.headphones * (255 - off) / 255;
  }
  if (pastime_ == Pastime::kPhonesOn) {  // coming down over its ears
    s.headphones = std::max(s.headphones, s.fade);
    s.drop = kPhonesDropPx - kPhonesDropPx * int(ease(std::min<uint32_t>(s.age, kPhonesOnMs), kPhonesOnMs)) / 1024;
  }
  s.zone = opacity(zone_);
  s.plant = opacity(plant_);
  s.grown = grown_;
  if (inZone_) s.mugs = 1 + int(std::min<uint32_t>(kMostMugs - 1, (t - zoneSince_) / kMugEveryMs));
  return s;
}

void GelFace::wish(const WishShow& show) {
  // A wish left: a silent sigh and slump as its bubble fades.
  if (show.pop == WishShow::kGone && wish_.pop != WishShow::kGone)
    if (const rig::Clip* c = rig::kRig.clip("wish-gone")) play(*c);
  wish_ = show;
}

void GelFace::play(const rig::Clip& clip) {
  mixer_.play(clip);
  cueing_ = &clip;
  cueAge_ = 0;
}

// The finger in the engine's body units, where the dome stands: kPxPerUnit
// canvas pixels a unit, its middle at (kMidX, kMidY), and y up, as the
// engine's is and the canvas's isn't. The panel draws the canvas half as
// large again, so the dome, 128 panel pixels a unit at its middle, goes
// with the finger.
void GelFace::finger(bool down, int x, int y) {
  if (!down) {
    letGo(0, 0);
    return;
  }
  if (!finger_) {
    finger_ = true, fingerX_ = x, fingerY_ = y;
    // Where on the dome, -1 … 1 across its half-width and half-height.
    engine_->dragStart(F(x - kMidX) / kPxPerUnit / q(1.1), F(kMidY - y) / kPxPerUnit / q(.91));
    return;
  }
  engine_->dragMove(F(x - fingerX_) / kPxPerUnit, F(fingerY_ - y) / kPxPerUnit, F(0));
}

void GelFace::letGo(int vx, int vy) {
  if (finger_) engine_->dragEnd(F(vx) / kPxPerUnit, F(-vy) / kPxPerUnit, F(0));
  finger_ = false;
}

// At every stop of Behaviour's, on the grid or not.
void GelFace::step(uint32_t t, linkkit::Rng&) {
  advanceTo(t);
  if (!stream_ || !stream_->drawn(t) || !stream_->call.live) return;
  const auto& c = stream_->call;
  if (c.at != landedAt_) landed_ = false, landedAt_ = c.at;
  if (!landed_ && int32_t(t - c.at) >= int32_t(callFlightMs(c.style))) {
    landed_ = true;
    land(c.style);
  }
}

// What the body does as a word lands, in the pull's body units a second:
// knocked back and dazed by a failure, longer up the ladder; a squash as a
// word sinks in or a bit of hail lands; stretched up by a shrug or a cheer.
void GelFace::land(CallStyle style) {
  auto& e = *engine_;
  // Toward the word's side: its lane's half of the screen.
  const int column = stream_ ? stream_->column(stream_->call.slot) : 0;
  const F side = stream_ && 2 * column + 1 >= stream_->laneCount ? F(1) : F(-1);
  switch (callMove(style)) {
    case CallMove::kHit:
      // A failure's knock; its dizzy eyes are the stream's to draw. Your
      // no makes it flinch and squeeze its eyes; a timeout lands on it
      // half asleep.
      if (style == CallStyle::kNo) e.jolt(q(-1.5), q(2.4), q(-2), q(.6), 450);
      else if (style == CallStyle::kTimeout) e.jolt(q(.6), q(2.8), q(-2), q(.9), 700);
      else e.jolt(q(1.2), q(3.2), q(-3));
      break;
    case CallMove::kSink: e.jolt(0, q(1.8), 0); break;
    case CallMove::kShatter: e.jolt(0, q(1.4), 0); break;
    case CallMove::kShrug: e.jolt(0, q(-1.6), 0); break;
    // The whole turn's: a gulp at its end; the cube's thud; buried, its
    // lids heavy; leaning in to poke a frozen word; a blink as all goes off.
    case CallMove::kGather: e.jolt(0, q(1.6), q(.6)); break;
    case CallMove::kCompact: e.jolt(q(.6), q(1.2), 0); break;
    case CallMove::kBuried: e.jolt(0, q(1.8), 0, q(.5), 900); break;
    case CallMove::kFreeze: e.jolt(side * q(.8), 0, q(.8)); break;
    case CallMove::kDrop: e.jolt(0, q(.6), 0, q(.6), 300); break;
  }
}

// Compares without building the engine's facts, so an unchanged state
// (every tick's sync) allocates nothing.
bool GelFace::factsChanged(const FaceContext& c) const {
  const auto& old = engine_->router.facts;
  char id[12];
  std::snprintf(id, sizeof id, "%lu", (unsigned long)c.attentionId);
  return old.base != c.base || old.act != c.act || old.attention.on != c.attention || old.attention.id != id ||
         old.attention.agent != c.agent || old.attention.project != c.project || old.attention.name != c.thread ||
         old.attention.more != c.more;
}

void GelFace::sync(const FaceSource& source, const FaceContext& context, uint32_t t) {
  advanceTo(t);
  pace_ = context.pace;
  Measure measure(gel::profile.sync);
  auto& e = *engine_;
  if (factsChanged(context)) {
    Facts f;
    f.base = context.base;
    f.act = context.act;
    f.attention = {context.attention, std::to_string(context.attentionId),
                   context.agent,     context.project,
                   context.thread,    context.more};
    const auto& old = e.router.facts.attention;
    // A request the alert hasn't shown: its knocks and ding are new.
    bool freshAttention = f.attention.on && (!old.on || old.id != f.attention.id || old.agent != f.attention.agent ||
                                             old.project != f.attention.project);
    if (freshAttention) restartCues();
    e.setFacts(f);
  }
  e.setMood(expressionFromName(moodName(context.mood)));
  bool timeout = source.look == SceneState::kNoApp;
  if (timeout != e.router.timedOut) e.linkTimedOut(timeout);
  e.setOccasion(context.when, context.lookMs);
  // The Mac's borrowed expression, else a gesture's or a notice's own face
  // while it plays. A low or cross mood keeps its own face through a
  // gesture.
  int borrowed = context.borrowed ? expressionFromName(moodName(source.mood)) : -1;
  if (borrowed < 0 && source.anim == SceneState::kGesture &&
      !slime::kKeepsFaceThroughGestures[profileFor(context.mood)])
    borrowed = slime::kGestureFaces[std::min<int>(source.animVariant, slime::kGestureCount - 1)];
  if (borrowed < 0 && source.anim == SceneState::kNotice)
    borrowed = slime::kNoticeFaces[std::min<int>(source.animVariant, slime::kNoticeCount - 1)];
  if (borrowed < 0 && stream_) borrowed = actFace(stream_->actNow(t), e.emotion);
  bool next = source.anim != SceneState::kNone && (source.anim != source_.anim || source.at != source_.at);
  if (next) {
    // Behaviour times the reaction (its loops, its line), so the engine
    // holds it until the source moves on.
    Request r;
    r.anim = source.anim;
    r.loops = source.loops;
    r.outcome = source.outcome;
    r.ctx = source.ctx;
    r.variant = source.animVariant;
    r.expression = borrowed;
    e.play(r);
    restartCues();
    if (const rig::Clip* clip = rig::kRig.clip(stateName(source.anim))) play(*clip);
  } else if (source.anim == SceneState::kNone && source_.anim != SceneState::kNone && !timeout) {
    e.clear();
    restartCues();
  }
  e.borrow(borrowed);
  // A reply to what you said: its clip over the face it came with, or the
  // pastime it asks for, once, as that face starts. A pack with none for
  // it plays the face alone.
  if (context.reply[0] && !(replied_ && context.replyAt == replyAt_)) {
    replied_ = true, replyAt_ = context.replyAt;
    reply(context.reply, source.anim, t);
  }
  if (source.state() != source_.state()) {
    lookAt_ = t;
    restartCues();
  }
  source_ = source;
  showLayers(t);
}

// The engine's shuffle bag for the reaction, drawn from copies: a call
// refused after its pick (Device::refuse) leaves the bag and the engine's
// randomness as they were. started draws the same take for real.
uint8_t GelFace::pick(SceneState a, Mood mood, int wanted, Outcome outcome, StartCtx ctx, linkkit::Rng&, int loops,
                      int turn) const {
  // A successful finish by its size: small deals Little Win's takes from
  // the bag, as every other row does.
  if (a == SceneState::kTaskComplete && outcome == Outcome::kSuccess) {
    const int canonical = slime::kProfile[profileFor(mood)].canonical;
    switch (finishSize(loops, turn, canonical)) {
      case FinishSize::kSmall: break;
      case FinishSize::kMedium: return uint8_t(finishVariant(Finish::kConfetti));
      case FinishSize::kBiggest: return uint8_t(finishVariant(Finish::kMegaBoop));
      case FinishSize::kBig:
        return uint8_t(finishVariant(
            bigFinish(canonical, slime::kProfile[engine_->emotion].canonical, wanted, lastBig_)));
    }
  }
  const int row = recipeState(a, outcome, ctx);
  auto bag = engine_->scenes.bags[slime::kProfile[profileFor(mood)].canonical][row];
  Random random = engine_->random;
  return uint8_t(bag.next(random, takes(row)));
}

void GelFace::started(SceneState a, Mood mood, uint8_t variant, Outcome outcome, StartCtx ctx) {
  if (a == SceneState::kGesture) return;
  if (a == SceneState::kTaskComplete && isFinishVariant(variant)) {
    // The big one it played, which the next big one isn't, where it can be.
    const Finish f = finishOf(variant);
    if (f != Finish::kConfetti && f != Finish::kMegaBoop) lastBig_ = f;
    return;
  }
  const int row = recipeState(a, outcome, ctx);
  engine_->scenes.bags[slime::kProfile[profileFor(mood)].canonical][row].next(engine_->random, takes(row));
}

int GelFace::plays(SceneState a, uint8_t variant, int loops) const {
  return a == SceneState::kTaskComplete && isFinishVariant(variant) ? 1 : loops;
}

int GelFace::variants(Mood, SceneState s) const { return s == SceneState::kGesture ? slime::kGestureCount : kTakes; }

uint32_t GelFace::loopMs(Mood mood, SceneState state, int variant) const {
  return uint32_t(F(recipe(mood, state, variant, Outcome::kNone, StartCtx::kNone).duration).integer());
}

// A line over a reaction starts at its first result contact (the card or
// the alert), less the speaker's lead.
uint32_t GelFace::voiceMs(Mood mood, SceneState state, int variant) const {
  const auto& r = recipe(mood, state, variant, Outcome::kNone, StartCtx::kNone);
  for (int i = 0; i < r.count; ++i)
    if (reveals(r.contacts[i].kind)) return uint32_t(F(r.contacts[i].at).integer() + 70);
  return 0;
}

uint32_t GelFace::holdMs(Mood, int loops, uint32_t) const {
  uint32_t loop = uint32_t(F(engine_->scenes.recipe.duration).integer()),
           age = uint32_t(F(engine_->scenes.ageMs).integer());
  return loop * loops - age % loop;
}

int GelFace::cues(uint32_t, bool, voice::FxEvent* out, bool& changed) {
  changed = revision_ != cueRevision_;
  cueRevision_ = revision_;
  int count = pendingCount_;
  for (int i = 0; i < count; ++i) out[i] = pending_[i];
  pendingCount_ = 0;
  return count;
}

// The lane is drawn at the panel's size too (render/gel/lane.h), so the
// canvas is left to the test pattern.
bool GelFace::draw(Canvas&, Surface* native, const SceneShow& show, uint32_t t, const char* bubble,
                   const char* caption, const SignShow& sign, const DoneShow& done, const Strip& strip, bool) {
  if (!native || !native->pixels) return false;
  const PastimeShow kit = stream_ && actTakesOver(stream_->actNow(t)) ? PastimeShow{} : pastimes(lastTick_);
  size_ = {native->width, native->height, native->corner};
  paintFrame(*native, *engine_, show.mouthOpen, show.dy, bubble, sign, strip, done, mixer_.channels(), &layers_,
             lastTick_, stream_, t, &kit, drawer_, caption, &wish_);
  return true;
}

int GelFace::profile(char* out, size_t n, uint32_t frames) {
  auto& p = gel::profile;
  if (!frames) return 0;
  int written = std::snprintf(
      out, n,
      "gel_parts engine_us=%lu sync_us=%lu body_us=%lu art_us=%lu layers_us=%lu props_us=%lu clear_us=%lu "
      "lane_us=%lu stream_us=%lu\n",
      (unsigned long)(p.engine / frames), (unsigned long)(p.sync / frames), (unsigned long)(p.body / frames),
      (unsigned long)(p.art / frames), (unsigned long)(p.layers / frames), (unsigned long)(p.props / frames),
      (unsigned long)(p.clear / frames), (unsigned long)(p.lane / frames), (unsigned long)(p.stream / frames));
  p = Profile{};
  return written > 0 && size_t(written) < n ? written : 0;
}

}  // namespace render::gel
