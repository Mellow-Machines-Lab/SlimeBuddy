// The work acts on the gel (render/stream.h's Stream::ActNow): a command
// worth naming (the tests, the build, a push), for as long as it runs. All
// the gel shows of one is the low sign: what's running, with dots, then how
// it ended, with a tick or a cross. The desk and the work stream stay as
// they are, and the body answers the result once (GelFace::answerAct).
#pragma once
#include "render/gel/low_sign.h"
#include "render/stream.h"

namespace render::gel {

// The low sign that says what's running, then how it ended.
LowSign actSign(const Stream::ActNow& act);

}  // namespace render::gel
