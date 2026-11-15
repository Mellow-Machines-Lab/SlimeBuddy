#include "render/gel/act.h"

#include "render/palette.h"

namespace render::gel {

LowSign actSign(const Stream::ActNow& act) {
  constexpr uint16_t kSage = rgb565(kSageRgb), kClay = rgb565(kClayRgb), kEye = rgb565(kEyeRgb);
  LowSign sign;
  if (!act.shows()) return sign;
  sign.text = act.text;
  if (act.result == ActResult::kRunning) {
    sign.mark = LowSign::Mark::kDots;
    sign.ink = kEye;
    sign.ms = uint32_t(act.ageMs);
  } else {
    sign.mark = act.result == ActResult::kPass ? LowSign::Mark::kTick : LowSign::Mark::kCross;
    sign.ink = act.result == ActResult::kPass ? kSage : kClay;
    sign.ms = uint32_t(kLowSignRiseMs);  // over the running one: the words change in its place
    sign.leftMs = act.leftMs;
  }
  return sign;
}

}  // namespace render::gel
