# Slime validation

Updated 2026-11-15. What checks the gel slime, and which rule each check
holds it to. The slime has one engine, the C++ one
(`character/firmware/src/render/gel/`), so every rule
[CONTINUITY.md](CONTINUITY.md) states is checked on it, from the
repository root:

```sh
node character/tools/slimegen/slimegen.mjs --check
make -C internal fw-test
internal/tools/boopctl sim --face gel
make -C internal mirror
```

None of them contacts Jev, a paid API or a device.

## The design's data (slimegen)

`slimegen.mjs` checks the design before it bakes it (`check.mjs`), and
`--check` fails unless every generated file is the one checked in:
`slime.h`, `slime_art.h` and `slime_mesh.h`. It
fails, naming what's wrong, when:

- a face the pack's `character.json` names has no expression of its own
  or no sound profile, or the first expressions aren't those faces; a
  lasting mood rests in a face there isn't, or a face is of no lasting
  mood;
- an expression hasn't three gestures or is of no face, a face names a
  bit there isn't, or a gesture or a notice take borrows an expression
  there isn't;
- two pastimes' bodies share an id, or one has neither a bit of its own
  nor a recipe;
- a sound sketch renders differently from one seed, doesn't start from
  silence, isn't finite, is louder than 0.8, longer than 3 s, or isn't
  silent at no gain;
- the gestures or the notices are out of `render::Gesture`'s order, a
  family a gesture's face gives way to has no mood.

## The engine (`test_gel`)

`make -C internal fw-test` runs the pack's `tests/firmware/test_gel/` on
the Mac (env `native`). Every frame its tests drive is held to the bounds
of what may be drawn (`frames.h`, CONTINUITY.md, Continuity of what is
drawn): every spring and channel inside its bounds, each art's weight and
drawn size, each prop and the desk moving no more in a frame than their
ramps and springs can, and no jolt at an event. By file:

- `test_cells.cpp`: every canonical mood in every state, each of its
  takes, driven as the board drives it (`test_every_cell_plays_truthfully`):
  the mood held, one reveal for each of a request's, the prop it needs
  shown, only a successful finish's trophy and proud's gift, quiet states
  silent but for comedy and still but for a snore, each take moving the
  body its own way; ten minutes at work; contacts a finger or an alert
  interrupts; asleep's stillness.
- `test_finish.cpp`: a successful finish's size by its loops and its
  turn's tier (small short of tier1, the confetti at tier1, Gold Rush from
  tier2, a sad one small); the face's pure pick, a finish above small played
  once, its length and its `say`'s start; one cheer and its bounces, never
  the fanfare; the desk put away by a takeover and no card; every finish
  moving smoothly and settling by its end; Gold Rush's flood above the
  lane and Little Win's trophy; only what draws behind the body erasing the
  frame whole first, less the rows the flood covers.
- `test_face.cpp`: the semantic face's spring, windows and saturation,
  and a rapid chain of faces settling on its last.
- `test_motion.cpp`: the accepted V1 springs, impulses, breath, blink,
  gaze and clock (the motion contract: changing the motion means editing
  this test); a finger's springs, reach and fling; acting's timing and
  springs; every setter keeping every spring, and an engine replaying the
  same from the same seed; the scenes' formulas (the waiting wave, the
  cards' and the error's, the recovery gesture, the desk's jitter, a bit's
  keys inside their bounds).
- `test_requests.cpp`: what routes when (no app, listening, needs you, a
  moment, the look), a request's identity, cues sparse and quiet, a
  request sounding once whatever changes, and sound's randomness changing
  nothing drawn.
- `test_hand.cpp`: your hand on the whole board, the device with the gel's
  face. A finger: the eyes following it and where a drag starts on the dome; at
  rest the lean, then the doze, and lifting it
  (`test_a_finger_at_rest_leans_then_dozes_and_lifting_wakes`,
  `test_lifting_a_doze_wakes_it_with_a_start`), which allocates nothing. What
  a hold leaves alone: a short one stays a tap, a stroke and a tickle stay
  theirs, the needs-you screen's only dismisses, the strip's is the drawer's,
  a finish's and an ask's keep their touches. What ends a lean: a needs-you,
  listening, a Moment. Its breath minding the volume, and a touch saying why
  itself, with the Mac's reason not doubling it. Tilt: the slosh and how it
  settles, giving way to whatever else moves the body, with no allocation.
  And the listening nod.
- `test_main.cpp`: the rest: arithmetic (a `Divisor`'s quotients exactly
  `/`'s), the binary64 clock's rounding, every expression's bounds,
  deterministic pixels, the body's outline soft with no seam inside, over
  the crown too, and holding still as the mesh turns
  (`test_the_outline_holds_still_as_the_mesh_turns`: leaned or squashed
  end to end, no row's edge jumps a pixel and a quarter), the material
  (1025 × 513 texels) drawing the same body from the AMOLED's copy
  (`test_the_materials_copy_draws_the_same_body`), request identity and bounded caches, alerts
  interrupted by listening or a link timeout at eight points signalling
  exactly once, the face's arts crossing over without jumps and never
  fading (its visible ink through five cross-fades), a face coming in or
  going moving nothing in the draw order (the nine mood changes that once
  flickered for a frame, `test_a_face_coming_in_moves_nothing`), falling asleep as a
  morph (the awake face squashing shut on its slow spring, to lines, the
  sleeping face crossing in once it's 0.9 shut, waking quick, sleep's
  spring kept through a mood change), the night's lids, a finger moving the dome its own
  way, a finished turn's
  glow (`test_a_finished_turn_glows`: its timing, strength and reach,
  nothing past the panel's corners) and low sign
  (`test_the_low_sign_stays_in_the_lane_and_holds_a_long_name`: a short
  name large, 50 characters whole on two lines, cut past that, nothing
  above the lane's rows, and its key steady at rest;
  `test_the_strip_gives_way_to_the_finishs_sign`), comedy, the ten gestures (a low or cross mood keeping its own face
  through one: `test_a_low_or_cross_mood_keeps_its_face_through_a_gesture`), the notice
  playing the rig's clip (`test_a_notice_plays_its_rig_clip`: perking up
  tall, off the ground and landing flat, its eyes and mouth, a sound at
  each cue, no bit), a reply's clip (`test_a_reply_plays_its_clip_once`:
  every reply has one, 1 to 1.8 s, its name fits the board's buffer, a
  shake of the head leans the body, once for each reply, nothing for a
  reply with no clip, from a `ReactionIn` too), the
  rig's stretch on both faces (a row of the gel's pastimes, over its
  pose, and on Pixel's rig face, faded out by a Moment), the
  pastimes (`test_pastimes.cpp`: each row's body in the pack and its
  turn, each with a clip drawing above the lane and over at its clip's
  end, 45 minutes at `tier2` dealing only working ones of `tier1` and
  `tier2`, after a rest of 20 to 45 s, never the same twice running, the
  newest tier's about 60% of the time, and none with no tier nor tag; the
  tagged pastimes, `test_a_tagged_pastime_plays_only_while_its_tag_is_on`: each
  only while its tag is on, about three deals in eight, idle's as acting's
  bit and work's and asleep's as the scene's take, busier typing only
  while busy and the dream after half an hour; where it looks and typing along (`test_gaze.cpp`: the rules' order, needs you before a pose and a pose's clip keeping the eyes from a glance, a glance's 1.6 s and its 7 s rest with at most seven glances in the busiest minute, a look slower than a finger's follow, and the keys' 4 s linger, no pastime over them, gone for needs you, work and sleep, never cutting a pastime, and at work only in its typing pose); none dealt while something
  needs you; the move, the melt, at least 5 minutes apart at `tier3`, the
  paws its while it plays; working's base by tier, its lids lower the further in and
  faded out under a Moment or while something needs you; and the paws
  lifting before each key contact and tapping softly between), the rig's
  `x`, `reach` and `melt` on the gel
  (`test_the_rig_carries_reaches_and_melts_the_gel`: x off the screen, the
  reach's side out and the other in, a melt most of the way down and
  wider, its base where it was),
  the lane at the panel's size (fading whatever the draw cadence, blended
  once, cut and marked as the canvas does), the working spinner a spoke
  every 125 ms and the lane drawn again for it only while sessions work
  (`test_the_working_spinner_turns_once_a_second`), the lane clear of the AMOLED's
  rounded corners (`test_the_lane_clears_the_rounded_corners`), the layers on the body's
  anchors, coming and going through the rig's layer stack, each at its
  opacity, the sign fading out at its own and, carried far to either
  side, held `SignLayer::kEdge` in from the panel's, drawing by coverage (a
  polygon's exact area in each pixel, so a lid 3/16 of a pixel thin never
  flickers, `test_polygon_edges_move_a_sixteenth_at_a_time`; arts of
  different shapes cross-fading with nothing showing through,
  `test_a_cross_fade_never_shows_through`, every two arts' in their room,
  `test_every_cross_fade_fits_its_room`), what moves gliding between
  pixels (`test_a_recording_replays_between_pixels`,
  `test_text_glides_between_pixels`, the outfit half a pixel down while
  the body moves), tracked
  clearing and kept cards and lanes matching a fresh frame, a finished
  turn's low sign and glow coming and going over them, a word falling over
  a kept card not staying in it
  (`test_what_falls_over_a_kept_card_doesnt_stay_in_it`), steady frames
  allocating nothing, and the work stream
  (`test_the_work_stream_falls_behind_and_hits_in_front`: words in their
  lanes' columns behind the body, a hit's knock and daze and its lids
  opening after, every style drawn off the heap), a hit's daze ending and
  forgotten across the engine clock's wrap
  (`test_a_daze_ends_across_the_clock_wrap`), the desk's turn clock in
  hours and minutes from the hour, never wrapping
  (`test_the_desk_clock_shows_hours_from_the_hour`), and the agents' drawer
  (`test_drawer.cpp`: a pull up the strip opening it and what the Mac
  hears, a swipe up the face staying the face's, a short pull springing
  back, a tap on a row picking it, shutting itself after 15 s and saying
  it's open till then, BOOT shutting it, its rows inside the panel's
  rounded corners, a long list's rows scrolling under a finger, gliding,
  springing back and resting on a whole row as the board asks for the
  rows it comes to, the body pressed flat under it, and its frames
  allocating nothing).

## The board's frames

- `internal/tools/boopctl sim --face gel` plays the pack's
  `tests/scenarios-gel/` against its `tests/golden-gel/`: every canonical
  mood and factual state, failure and neutral-finish frames, the host lane
  at the panel's size (`lane.jsonl`: the counts in words and as marks, a
  long name that needs you, cut with its "…" and how many more in a pill, a
  short one whole with the marks and no helpers or to-do list, the
  finish's marks, one with no thread, the bubble in its fonts, the
  caption after the words, an ask's answers, needs you with today's ring
  up, a finish with words and a caption, and the needs-you card for each
  kind and for none), the
  comedy (`comedy.jsonl`: the yawn and the sneeze's wind-up, asked for, the
  long yawn, the zoomies at either edge, a snore, the hiccups and the
  gift), the gestures (`gestures.jsonl`: BOOT handing you an agent, a stroke, a boop
  on the nose, a shake and peekaboo, each by the board's own input, its
  variation expected), the tagged pastimes (`occasions.jsonl`: frantic typing, asked for by name, and
  `tier3`'s bare desk), the pastimes (`pastimes.jsonl`: the bare desk at
  `tier3`, 20 s in and later, the done beat's paw on Enter and its sitting
  back, a stretch, a puddle mid-melt, idle with nothing beside it, a look
  around, a hopeful look, a hop and a snooze's Zs, asleep a snore's bubble
  and a roll over, and needs you with no pastime after two minutes), the
  replies (`replies.jsonl`: each reply's clip partway through under its
  face and sign, the hush reply's squeeze and nod among them, a no over the
  needs-you screen, the slight wave at a lock and the bye at a night's
  lock), the
  gaze (`gaze.jsonl`: at work typing, forward, a glance at a word and down it, forward again and no glance inside its rest; looking, running and waiting with the same word falling and no glance; needs you up at you, then turned to the Mac on either side and held, forward with no side), typing along (`along.jsonl`: the keys out and each paw tapping, paused, put away, gone under a Moment, at work each brisk paw with its eyes down and back to soft when you stop, its paws off the keys while it runs or looks though you type and typing with you again after, nothing asleep), the
  layers (`layers.jsonl`: the nightcap at night and on a party night, Zs
  asleep at night, a morning and each special day with nothing worn and
  nothing behind or round it, a sign under the nightcap), a Moment's sign (`sign.jsonl`), a finished
turn's glow and low sign as a face for it talks, lingers, fades, falls
and is taken down, and the sign by its name and a big cheer's (`done.jsonl`), the
  four activities (`activities.jsonl`: each pose from plain working at
  0.2, 0.6, 1, 1.6 and 2.4 s, the three typing tiers, and four changes of
  pose a frame every 50 ms), the
  work acts (`acts.jsonl`: the low sign running with the words still
  falling, a pass with its hop, a fail with its flinch, a push shipped; a
  needs-you and back), the
  work stream (`stream.jsonl`: two lanes' words falling, nothing of them on
  the monitor, 20 s quiet with nothing beside it, and every style as it
  lands: bonk and its dizzy eyes, bonk3, shrug, shatter, helper and back
  (landing as a word does), an edit, and the whole turn's moments: a drop,
  and a turn's end gathering its words after the stream goes), and
  a finger in idle (`swipes.jsonl`), and the agents' drawer
  (`drawer.jsonl`: shut with the strip's grabber, pulled part way, open
  with the body squeezed flat and its paw over the edge, a row picked,
  handed over, and a long list scrolled, a row still coming as it
  scrolls). `--accept` updates only the gel
  goldens, after looking at them.
- `boopctl mirror` (in `make -C internal mirror`) plays the same scenarios
  on the Mac's mirror of the board (`character/mac/mirror/board.js`)
  and on the native simulator, both with no card, and checks every shot is
  the same, pixel for pixel; `firmware/tools/mirror.sh --check` builds the
  mirror again and checks it's the one checked in.
- `test_gel`'s `test_a_helper_draws_the_same_body` draws the body with a
  stand-in for the AMOLED's second core, its rows shared out differently
  every frame, and checks the frame is the same pixels as one core's.
- On the AMOLED, `internal/tools/boopctl perf --scenes` with the
  `amoled206_profile` build: each scene's frames a second against its
  budget, and the body's time (`body_us`, the erase included); its
  `stream` scene keeps two lanes' words falling and a word coming forward
  every 2 s (`stream_us`).
- `internal/tools/fluid/measure.py` measures how fluid the everyday
  transitions are on the board (`transitions.jsonl`, `idle-swipes.jsonl`),
  and the Character Studio's Transitions view ranks them in the page.

## For the eye

The Character Studio's **Gel on the board** card is the board's firmware:
its **Tools** record the review set (24 representative activities,
`character/studio/review-sets.json`) from the
board as silent WebM clips, and play the sound sketches each on its own.
The studio's server saves the clips only with `--capture-dir DIR`, only
those the pack names, from its own page with its token, at most 8 MiB
each, never over a file (`characters/studio/tests/`). `node
characters/studio/check.cjs` records one in a headless browser
([the studio's README](https://github.com/Mellow-Machines-Lab/buddygotchi/blob/main/characters/studio/README.md), Checking it). Clips are
art evidence, not owner approval.

## Limits that remain

- V3 artwork and the SFX sketches need the owner's art and listening
  review on the board.
- The owner approved the representative set before the cut to 18 faces;
  curious's new face needs the owner's look on the board.
- Real touch hardware and physical audio latency are separate tests, and
  so is the board's frame time (`boopctl ping`'s `draw_us`).
