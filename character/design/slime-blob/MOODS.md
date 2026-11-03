# Slime mood inventory and directed traversal

Generated from [emotion-graph.json](emotion-graph.json), [expressions.json](expressions.json)
and the pack's [character.json](../../character.json) by `node character/tools/slimegen/slimegen.mjs`;
edit those, not this.

Ordinary neighbors still require context/pacing. Dramatic neighbors require fresh
strong edge-specific evidence. Staying is implicit at every node. Families and
intensity labels are static design metadata, not LLM-selected numeric controls.
Faces are V3 review candidates; body descriptions below are choreography targets.

## Settled / low stimulation

### Calm (`calm`)

**Meaning:** Settled, the resting mood: nothing much is happening.

**Strength / activation / persistence:** baseline / low / stable baseline candidate.

**Face:** Quiet green neutral reference: two tiny egg-shaped eyes and one delicate o; no human eyebrows.

**Body intent:** Broad settled dome; even subtle breathing and slow blinks. No attention-demanding loop.

**Current gesture recipes:** Look aside and return (3.6 s); Small settling nod (2.4 s); Round and settle (3.2 s).

**Entry evidence:** Quiet recovery with no new relevant stimulus.

**Ordinary →** `comfy`, `curious`, `engaged`, `happy`, `lazy`, `annoyed`, `lonely`.

**Dramatic →** `surprised`, `wounded`, `excited`, `sad`.

### Comfy (`comfy`)

**Meaning:** Cosy: all is quiet, and it feels safe and snug.

**Strength / activation / persistence:** low / low / stable baseline candidate.

**Face:** Golden comfy reference: relaxed short bean lids and a very small open smile; face feels nestled.

**Body intent:** Gentle settled squish with a long recovery; small contented sway.

**Current gesture recipes:** Contented side sway (3 s); Long squishy exhale (4 s); Soft inward snuggle (3.2 s).

**Its own comedy, idle only:** Long slow yawn (4.2 s).

**Entry evidence:** Settled safe context or explicitly friendly interaction.

**Ordinary →** `calm`, `lazy`, `happy`, `lonely`.

**Dramatic →** `surprised`.

### Lazy (`lazy`)

**Meaning:** Can't be bothered: low effort for show, though the work still gets done.

**Strength / activation / persistence:** low / low / sustained expressive mood.

**Face:** Mint lazy reference: two chunky parallel rounded lids per eye, with a soft w—not a flat bored stare.

**Body intent:** Languid sideways sag and a slow anticipatory lean before a correct action.

**Current gesture recipes:** Droop and recover (3.8 s); Look aside and return (3.6 s); Sleepy slow nod (4.4 s).

**Its own comedy, idle only:** Long slow yawn (4.2 s).

**Entry evidence:** Low-stakes idle characterization; never a reason to delay the real task.

**Ordinary →** `comfy`, `calm`, `engaged`, `lonely`.

**Dramatic →** `surprised`.

### Bored (`bored`)

**Face only:** settles into `lazy`.

**Strength / activation / persistence:** low / low / sustained expressive mood.

**Face:** Lilac bored reference: low heavy oval beans, compact pout; avoid long angry bars.

**Body intent:** Low sag, slow sideways glance and a tiny flattening sigh; no fake progress or repeated alerts.

**Current gesture recipes:** Lean back and side-eye (2.8 s); Droop and recover (3.8 s); Long squishy exhale (4 s).

**Its own comedy, idle only:** Long slow yawn (4.2 s).

**Entry evidence:** Opted-in low-stimulation acting, not inferred abandonment.

## Energy / fatigue

### Tired (`tired`)

**Face only:** settles into `lazy`.

**Strength / activation / persistence:** mild / low / sustained expressive mood.

**Face:** Long drooping, round-tipped lids and a tiny oval yawn; sleepy rather than sorrowful.

**Body intent:** Longer blinks, a small yawn squash and reduced movement amplitude while remaining productive.

**Current gesture recipes:** Stretchy yawn (4.2 s); Sleepy slow nod (4.4 s); Droop and recover (3.8 s).

**Its own comedy, idle only:** Long slow yawn (4.2 s).

**Entry evidence:** Opted-in sustained-effort acting with a quieter performance budget.

### Exhausted (`exhausted`)

**Face only:** settles into `lazy`.

**Strength / activation / persistence:** strong / very low / sustained expressive mood.

**Face:** Wider, lower soft hanging lids and a flattened exhausted pout; no hostile eyebrows.

**Body intent:** Broad soft collapse, drooping face and a very slow elastic return; strong intensity with low activation.

**Current gesture recipes:** Slow jelly collapse (5 s); Sleepy slow nod (4.4 s); Long squishy exhale (4 s).

**Entry evidence:** Stronger opted-in fictional effort acting; not actual resource failure.

## Joy / humor

### Pleased (`pleased`)

**Face only:** settles into `happy`.

**Strength / activation / persistence:** mild / medium / stable baseline candidate.

**Face:** Blue pleased reference: substantial inward smiling bean eyes, round w smile and tiny cheek freckles.

**Body intent:** Tiny buoyant lift, then a comfortably rounded settle.

**Current gesture recipes:** Single buoyant hop (2.4 s); Contented side sway (3 s); Soft inward snuggle (3.2 s).

**Entry evidence:** Mild positive progress or a friendly acknowledged interaction.

### Happy (`happy`)

**Meaning:** Glad: things are going well, or something small went right.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** Blue happy reference: round ink dots and a pale-filled, round-bottom U smile, no teeth or complicated lip.

**Body intent:** A soft whole-body upward bounce with restrained overshoot.

**Current gesture recipes:** Single buoyant hop (2.4 s); Two eager hops (3 s); Purposeful bob (2.5 s).

**Entry evidence:** Clear positive interaction or supported welcome progress.

**Ordinary →** `excited`, `proud`, `curious`, `comfy`, `calm`, `engaged`, `annoyed`.

**Dramatic →** `sad`, `surprised`, `wounded`.

### Excited (`excited`)

**Meaning:** Thrilled: a big piece of work just finished well.

**Strength / activation / persistence:** strong / high / sustained expressive mood.

**Face:** Teal excited reference plus original Rimuru smiling eye arcs: long, soft upward closed eyes and a wide bean laugh.

**Body intent:** Two short buoyant compress-and-lift beats with lively but bounded wobble, then continued activity.

**Current gesture recipes:** Two eager hops (3 s); Laughing belly jiggle (3.6 s); Happy side-to-side wiggle (2.6 s).

**Its own comedy, idle only:** Zoomies, edge to edge (2.6 s).

**Entry evidence:** Fresh unusually positive progress, playful poke scene or earned celebration.

**Ordinary →** `happy`, `proud`, `curious`, `determined`.

**Dramatic →** `surprised`, `grumpy`, `wounded`, `sad`.

### Amused (`amused`)

**Face only:** settles into `happy`.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** One relaxed smiling eyelid, one cheeky little egg eye, and an off-centre bean grin.

**Body intent:** A one-sided lift and one or two compact chuckle-like compressions; warmer than mischievous.

**Current gesture recipes:** One-sided chuckle (2.8 s); Wink and tilt (2.5 s); Contented side sway (3 s).

**Entry evidence:** An explicitly funny or playful context.

## Interest / understanding / focus

### Curious (`curious`)

**Meaning:** Interested: something new or puzzling, or a poke.

**Strength / activation / persistence:** mild / medium / sustained expressive mood.

**Face:** Neutral/curious reference retained: unequal soft dot sizes trade smoothly, with a tiny questioning o; no stern brows.

**Body intent:** Head/body tilt and inspect; unequal eye shapes change deliberately, not anxious tremors.

**Current gesture recipes:** Lean in and inspect (3.4 s); Double-take and pause (3.2 s); Avert, then peek (3.4 s).

**Entry evidence:** New topic, unresolved detail or an actual investigative activity.

**Ordinary →** `calm`, `engaged`, `happy`, `annoyed`, `surprised`, `excited`, `determined`.

**Dramatic →** `wounded`, `sad`.

### Engaged (`engaged`)

**Meaning:** Absorbed: steady work is going well.

**Strength / activation / persistence:** moderate / medium / stable baseline candidate.

**Face:** Two even, taller soft egg eyes and a compact settled smile; alert without looking robotic.

**Body intent:** Small steady task-oriented pulses; body and prop remain continuous.

**Current gesture recipes:** Purposeful bob (2.5 s); Lean in and inspect (3.4 s); Grounded effort brace (3 s).

**Entry evidence:** Sustained purposeful activity with no fresh obstruction.

**Ordinary →** `calm`, `curious`, `determined`, `happy`, `annoyed`, `lazy`, `proud`.

**Dramatic →** `excited`, `sad`.

### Determined (`determined`)

**Meaning:** Pushing on: the work is hard, or a retry is under way.

**Strength / activation / persistence:** strong / high / sustained expressive mood.

**Face:** Subtle upward brows and attentive ink eggs; a little determined mouth, never a villain glare.

**Body intent:** Grounded forward lean, stronger deliberate effort pulses and controlled recoil.

**Current gesture recipes:** Grounded effort brace (3 s); Purposeful bob (2.5 s); Expectant forward lean (3.2 s).

**Entry evidence:** Known challenge with a next safe step.

**Ordinary →** `engaged`, `proud`, `annoyed`, `irritated`, `sad`, `happy`.

**Dramatic →** `grumpy`, `excited`.

### Confused (`confused`)

**Face only:** settles into `curious`.

**Strength / activation / persistence:** mild / medium / sustained expressive mood.

**Face:** One tall and one squat egg eye, one soft short lifted brow and an angled tiny o.

**Body intent:** A modest alternating tilt and one deliberate reinspection; preserve composure.

**Current gesture recipes:** Questioning tilt (3.4 s); Double-take and pause (3.2 s); Lean in and inspect (3.4 s).

**Entry evidence:** Conflicting or unclear evidence, preferably an actual question.

### Baffled (`baffled`)

**Face only:** settles into `annoyed`.

**Strength / activation / persistence:** strong / medium to high / brief reaction with validated exit.

**Face:** Comically mismatched filled eggs and a small triangle mouth; no empty mechanical rings.

**Body intent:** A more conspicuous double-take, uneven tilt and short halt; high bewilderment without fear tears.

**Current gesture recipes:** Double-take and pause (3.2 s); Questioning tilt (3.4 s); Surprise rise and freeze (3.5 s).

**Entry evidence:** Fresh unresolved conflicting evidence, not the same line replayed.

### Skeptical (`skeptical`)

**Face only:** settles into `annoyed`.

**Strength / activation / persistence:** mild to moderate / medium / sustained expressive mood.

**Face:** One soft shelf-like lid and one curious bean; minimal side pout, teasing rather than contemptuous.

**Body intent:** Lean back, half-close one eye, then lean in to inspect evidence.

**Current gesture recipes:** Lean back and side-eye (2.8 s); Lean in and inspect (3.4 s); Cautious recoil (3.1 s).

**Entry evidence:** An explanation needing verification, not inferred distrust of a person.

## Confidence / mischief

### Proud (`proud`)

**Meaning:** Proud: something hard finally worked.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** Dark proud reference: short upward slanted confident eye beans and a deliberate curling w/smirk.

**Body intent:** Rise a little taller, pause to present the result and give a self-satisfied side sway.

**Current gesture recipes:** Rise and present (3.5 s); Contented side sway (3 s); Taller smug pose (3.7 s).

**Entry evidence:** Supported achievement or explicit praise; not unverified success.

**Ordinary →** `happy`, `engaged`, `determined`, `excited`, `annoyed`.

**Dramatic →** `wounded`, `comfy`, `grumpy`, `sad`.

### Mischievous (`mischievous`)

**Face only:** settles into `happy`.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** Green naughty reference: tiny cheeky diagonal bean eyes and a recognisable rounded 3 mouth.

**Body intent:** Sly lateral lean, tiny elastic feint and compact playful recoil. Actual tool actions remain untouched.

**Current gesture recipes:** Playful elastic feint (2.5 s); Avert, then peek (3.4 s); Wink and tilt (2.5 s).

**Entry evidence:** Explicitly playful context that does not alter authorized work.

## Social warmth / self-consciousness

### Shy (`shy`)

**Face only:** settles into `comfy`.

**Strength / activation / persistence:** mild / medium / sustained expressive mood.

**Face:** Pink shy reference: gently pinched closed eyes and a very small v, carried by generous oval blush.

**Body intent:** Small inward squish, avert then peek back; blush changes slowly, not flashing.

**Current gesture recipes:** Shrink, hide and peek (4 s); Soft inward snuggle (3.2 s); Avert, then peek (3.4 s).

**Entry evidence:** Welcome praise or gentle direct attention.

### Embarrassed (`embarrassed`)

**Face only:** settles into `comfy`.

**Strength / activation / persistence:** moderate / medium / brief reaction with validated exit.

**Face:** Smaller downward-averted eggs and an uneven sheepish w; softer than an alarm reaction.

**Body intent:** Retract slightly, compress inward and glance away before returning to the task.

**Current gesture recipes:** Shrink, hide and peek (4 s); Cautious recoil (3.1 s); Droop and recover (3.8 s).

**Entry evidence:** A fresh own correction or explicitly playful social fumble.

### Affectionate (`affectionate`)

**Face only:** settles into `comfy`.

**Strength / activation / persistence:** warm / low to medium / sustained expressive mood.

**Face:** Original Rimuru soft closed arcs with a warm little w and broad, soft blush cheeks.

**Body intent:** Warm forward lean and one soft nuzzle-like sway without hands, hearts or invasive UI by default.

**Current gesture recipes:** Soft inward snuggle (3.2 s); Expectant forward lean (3.2 s); Contented side sway (3 s).

**Entry evidence:** A welcome friendly or appreciative interaction.

### Lonely (`lonely`)

**Meaning:** Rare. Missing you, only at an idle check-in when CONTEXT says its wish is waiting 1 h or more, or when you come back after 20 to 40 hours away.

**Strength / activation / persistence:** mild to moderate / low / sustained expressive mood.

**Face:** Small lifted inner bean eyes and a vulnerable tiny pout; no accusing eyebrows or large sobs.

**Body intent:** Brief look toward the edge and a small pout, then settle; silent by default and never a persistent abandonment performance.

**Current gesture recipes:** Look aside and return (3.6 s); Avert, then peek (3.4 s); Droop and recover (3.8 s).

**Entry evidence:** Back after 20 to 40 hours away, or two hours of work with no touch or word (CONTEXT says so, at an idle check-in); never a quiet spell alone, a night away, days away, or while an agent works.

**Ordinary →** `sad`, `calm`, `comfy`.

**Dramatic →** `happy`.

## Frustration / anger

### Annoyed (`annoyed`)

**Meaning:** A little put out: a small failure, or an interruption.

**Strength / activation / persistence:** mild / medium / sustained expressive mood.

**Face:** Soft uneven low beans and a pushed-aside pout: restrained, not angry.

**Body intent:** One delayed correction, small flattening and restrained recoil; comparatively controlled.

**Current gesture recipes:** Lean back and side-eye (2.8 s); Clipped impatient correction (2.5 s); Cautious recoil (3.1 s).

**Entry evidence:** A small relevant obstacle, not the user's intent.

**Ordinary →** `calm`, `engaged`, `irritated`, `sad`, `surprised`, `determined`.

**Dramatic →** `grumpy`, `wounded`.

### Irritated (`irritated`)

**Meaning:** Losing patience: problems keep coming.

**Strength / activation / persistence:** moderate / high / sustained expressive mood.

**Face:** Slightly more inward-tilted, chunky eye beans with a short reluctant pout; no thin angular eye construction.

**Body intent:** Short uneven corrections, a tiny lid twitch and a stiffer spring return.

**Current gesture recipes:** Clipped impatient correction (2.5 s); Uneven frustrated twitch (2.3 s); Grounded effort brace (3 s).

**Entry evidence:** Fresh repeated relevant obstacles, not duplicate hooks.

**Ordinary →** `annoyed`, `grumpy`, `determined`, `sad`, `engaged`.

**Dramatic →** `wounded`, `calm`, `proud`.

### Grumpy (`grumpy`)

**Meaning:** Fed up: trouble keeps piling up.

**Strength / activation / persistence:** strong / high / sustained expressive mood.

**Face:** Big round-ended sulking slant eyes and a little m-shaped baby pout; bad temper stays adorable.

**Body intent:** Repeated compact forceful impacts at authored cue times; sustained disgruntled momentum, not endless dense SFX.

**Current gesture recipes:** Fast squash impacts (2.7 s); Brace, protest and recoil (3.2 s); Emphatic no-no wobble (2.6 s).

**Entry evidence:** Sustained fictional frustration with an obstacle while work continues.

**Ordinary →** `irritated`, `sad`, `determined`, `annoyed`.

**Dramatic →** `wounded`, `surprised`, `calm`, `proud`.

### Angry (`angry`)

**Face only:** settles into `grumpy`.

**Strength / activation / persistence:** extreme / high / sustained expressive mood.

**Face:** Squeezed, rounded >.< eye curves and a small comic protest mouth; do not stack sharp brows on tiny pupils.

**Body intent:** One concentrated brace-and-outburst gesture, followed by forceful settling; props may theatrically recoil or break.

**Current gesture recipes:** Brace, protest and recoil (3.2 s); Fast squash impacts (2.7 s); Grounded effort brace (3 s).

**Entry evidence:** A fresh strong fictional frustration cue; never attack the user.

## Disappointment / sorrow / support-seeking

### Disappointed (`disappointed`)

**Face only:** settles into `sad`.

**Strength / activation / persistence:** mild / low / sustained expressive mood.

**Face:** Gentle flattened drooping lids and a tiny downturned mouth; the shape reads as a letdown, not a scowl.

**Body intent:** A single slow deflation and downward glance; held smaller volume illusion without full crying.

**Current gesture recipes:** Droop and recover (3.8 s); Long squishy exhale (4 s); Look aside and return (3.6 s).

**Entry evidence:** A supported expectation falls short.

### Sad (`sad`)

**Meaning:** Down: the work failed, or was given up.

**Strength / activation / persistence:** moderate / low / sustained expressive mood.

**Face:** Rimuru sad eye curves: long soft drooping lids, a timid small pout and a little gathering water.

**Body intent:** A drooping settled dome, small downward sway and occasional liquid tears.

**Current gesture recipes:** Soft sob and settle (3.8 s); Droop and recover (3.8 s); Look aside and return (3.6 s).

**Entry evidence:** Fresh clear disappointment or opted-in character sorrow.

**Ordinary →** `wounded`, `calm`.

**Dramatic →** `grumpy`, `determined`, `happy`.

### Crying (`crying`)

**Face only:** settles into `sad`.

**Strength / activation / persistence:** strong / medium / sustained expressive mood.

**Face:** Purple crying reference: drooping closed lids over broad, rounded tear ribbons with a small wobbling pout.

**Body intent:** Broader tear flow and intermittent sob-like compressions; liquid tears follow the body surface, not pixel blocks.

**Current gesture recipes:** Soft sob and settle (3.8 s); Two tearful hiccups (3.2 s); Droop and recover (3.8 s).

**Entry evidence:** Stronger fresh character sorrow; never coerce attention.

### Whiny (`whiny`)

**Face only:** settles into `sad`.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** A baby-like pleading inward eye angle, watery lower lids and a little open pout; not empty white circular eyes.

**Body intent:** Outward pleading lean and nasal-looking pout pulses; continues the same task.

**Current gesture recipes:** Pouting forward plea (3.2 s); Two tearful hiccups (3.2 s); Emphatic no-no wobble (2.6 s).

**Entry evidence:** Explicitly comic request for sympathy or frustration-to-pleading transition.

### Hurt (`wounded`)

**Meaning:** Hurt: unkind words, or a failure after a lot of work.

**Strength / activation / persistence:** strong / low / sustained expressive mood.

**Face:** Green hurt reference: small vertical oval eyes, tender tear trails and a fragile rounded pout.

**Body intent:** An inward protective flinch, small retreat and hesitant glance back; softer than anger, more guarded than sadness.

**Current gesture recipes:** Protective flinch (2.8 s); Shrink, hide and peek (4 s); Droop and recover (3.8 s).

**Entry evidence:** Explicit character hurt or a clear opted-in social scene; not a denial of permission.

**Ordinary →** `sad`, `comfy`, `calm`, `surprised`, `annoyed`.

**Dramatic →** `grumpy`, `happy`, `determined`.

## Uncertainty / fear

### Uneasy (`uneasy`)

**Face only:** settles into `annoyed`.

**Strength / activation / persistence:** mild / medium / sustained expressive mood.

**Face:** Raised soft egg eyes and a hesitant little waver; keeps neutral identity but adds uncertainty.

**Body intent:** Cautious scan and small bracing lean; uncertain but still composed.

**Current gesture recipes:** Cautious left-right scan (3.2 s); Grounded effort brace (3 s); Cautious recoil (3.1 s).

**Entry evidence:** Unresolved uncertainty that is relevant to the character.

### Scared (`scared`)

**Face only:** settles into `surprised`.

**Strength / activation / persistence:** moderate / high / sustained expressive mood.

**Face:** Green scared reference: large pale rounded water eyes, little outer tear beads, and a wavy baby mouth; pupil-free.

**Body intent:** Tight brace, quick small glance and a light tremble; able to act between shivers.

**Current gesture recipes:** Brief bounded shiver (3.3 s); Protective flinch (2.8 s); Cautious left-right scan (3.2 s).

**Entry evidence:** Explicit alarm-like fictional situation; ordinary tests failing are not danger.

### Frightened (`frightened`)

**Face only:** settles into `surprised`.

**Strength / activation / persistence:** strong / high / sustained expressive mood.

**Face:** Light blue frightened: wide dark eyes with tears welling over them, and a trembling inverted arch; no angular glove shapes.

**Body intent:** Stronger bounded tremble, protective inward squash and broad watery eyes; no full-screen flashing.

**Current gesture recipes:** Brief bounded shiver (3.3 s); Shrink, hide and peek (4 s); Protective flinch (2.8 s).

**Entry evidence:** Strong explicit character alarm or approved exaggerated poke reaction.

## Surprise / freeze

### Surprised (`surprised`)

**Meaning:** Surprised, for a moment: something unexpected happened.

**Strength / activation / persistence:** mild / high / brief reaction with validated exit.

**Face:** Round ink dots that widen, and a rounded o; still a childlike slime rather than a human alarm face.

**Body intent:** A quick lift, pause and short wobble; cause-neutral until the evidence resolves.

**Current gesture recipes:** Double-take and pause (3.2 s); Single buoyant hop (2.4 s); Surprise rise and freeze (3.5 s).

**Entry evidence:** A genuinely new unexpected relevant event.

**Ordinary →** `curious`, `happy`, `annoyed`.

**Dramatic →** `grumpy`.

### Stunned (`stunned`)

**Face only:** settles into `surprised`.

**Strength / activation / persistence:** strong / high_then_low / brief reaction with validated exit.

**Face:** Shook reference: small horizontal pale ovals and a round-corner little triangular mouth; frozen but cute.

**Body intent:** A larger double-take followed by a short suspended pose; no continuous panic shake.

**Current gesture recipes:** Surprise rise and freeze (3.5 s); Double-take and pause (3.2 s); Cautious recoil (3.1 s).

**Entry evidence:** A stronger unexpected event justifying a temporary freeze.

### Speechless (`speechless`)

**Face only:** settles into `surprised`.

**Strength / activation / persistence:** moderate / low / brief reaction with validated exit.

**Face:** Purple unpleasant stare reference: broad chunky T-like lids with small stems and a tiny caret mouth.

**Body intent:** A flat still stare with one tiny hesitant settling motion; context determines blank versus displeased variant.

**Current gesture recipes:** Surprise rise and freeze (3.5 s); Lean back and side-eye (2.8 s); Look aside and return (3.6 s).

**Entry evidence:** An explicitly awkward or astonishing moment; not every ordinary wait.

## Anticipation / release

### Hopeful (`hopeful`)

**Face only:** settles into `happy`.

**Strength / activation / persistence:** mild to moderate / medium / sustained expressive mood.

**Face:** Open tall ink eggs and a short lifted smile, with a slight softened inner tilt; positive anticipation, not surprise.

**Body intent:** Slight attentive forward rise, waiting with an expectant small spring pulse.

**Current gesture recipes:** Expectant forward lean (3.2 s); Single buoyant hop (2.4 s); Avert, then peek (3.4 s).

**Entry evidence:** A plausible next step before the outcome is known.

### Relieved (`relieved`)

**Face only:** settles into `happy`.

**Strength / activation / persistence:** release, not an intensity rung / falling / brief reaction with validated exit.

**Face:** Long closed smiling arcs and a tiny soft open exhale; no broad laughter mouth.

**Body intent:** A long release from bracing, broad exhale-like squash and smooth return to the same body.

**Current gesture recipes:** Long squishy exhale (4 s); Contented side sway (3 s); Round and settle (3.2 s).

**Entry evidence:** A previously known obstruction is now resolved with fresh host evidence.

## Discomfort / aversion / overload

### Uncomfortable (`uncomfortable`)

**Face only:** settles into `annoyed`.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** Purple sick/uncomfortable reference: softly slanted pale almonds with a minimal wavering mouth; no clinical framing.

**Body intent:** A contained wince and uneven constrained squish; no illness metaphor in host status.

**Current gesture recipes:** Wince and withdraw (3 s); Cautious recoil (3.1 s); Questioning tilt (3.4 s).

**Entry evidence:** Explicit awkward/aversive context or a character-level discomfort cue.

### Disgusted (`disgusted`)

**Face only:** settles into `annoyed`.

**Strength / activation / persistence:** moderate / medium / sustained expressive mood.

**Face:** Comically squeezed soft eyes and a small asymmetric downturned mouth; recoil without cruelty.

**Body intent:** Compact recoil away from the troublesome prop, a squint and a springy recovery.

**Current gesture recipes:** Wince and withdraw (3 s); Cautious recoil (3.1 s); Emphatic no-no wobble (2.6 s).

**Entry evidence:** A comically messy bug or prop situation, never a person or identity group.

### Overwhelmed (`overwhelmed`)

**Face only:** settles into `engaged`.

**Strength / activation / persistence:** strong strain / high / sustained expressive mood.

**Face:** Unequal soft egg eyes, a little jelly-wavy mouth and small stress accents; remove the bug-eyed ring stare.

**Body intent:** Brief uneven compressions and a fast uncertain scan, then a bracing hold; no strobe or frantic constant audio.

**Current gesture recipes:** Cautious left-right scan (3.2 s); Uneven frustrated twitch (2.3 s); Grounded effort brace (3 s).

**Entry evidence:** Relevant bursts of concurrent activity or complexity, not simply one MCP call.

## Idle comedy every mood shares

The face deals these as pastimes from `tier1` (render/gel/pastimes.cpp; CONTINUITY.md, Comedy):

- `yawn-and-settle`: Yawn and settle (2.9 s).
- `sneeze-and-shake`: Sneeze and shake it off (2.7 s).

## Reference face alternates (not graph nodes)

- `comfy:rosy` → mood `comfy`: Pink comfy reference: tiny smiling diagonal beans, little triangle mouth, wide rosy cheeks; same comfy mood.
- `speechless:tearful` → mood `speechless`: Blue tearful speechless reference: small soft horizontal pale water eyes, short dash mouth and straight tears.

## Supplied references and their mappings

- Pleased (Happy Default) → expression `pleased`, mood `pleased`.
- Idle / Curious (Neutral Default) → expression `calm`, mood `calm` (related `curious`).
- Happy → expression `happy`, mood `happy`.
- Shy → expression `shy`, mood `shy`.
- Relaxed / Comfy → expression `comfy`, mood `comfy`.
- Happy / Excited → expression `excited`, mood `excited`.
- Relaxed / Comfy 2 → expression `comfy:rosy`, mood `comfy`.
- Bored / Unpleased → expression `bored`, mood `bored`.
- Lazy (Neutral) → expression `lazy`, mood `lazy`.
- Sick / Uncomfortable → expression `uncomfortable`, mood `uncomfortable`.
- Proud → expression `proud`, mood `proud`.
- Naughty? → expression `mischievous`, mood `mischievous`.
- Sad / Crying → expression `crying`, mood `crying` (related `sad`).
- Speechless (tearful) → expression `speechless:tearful`, mood `speechless`.
- Shook → expression `stunned`, mood `stunned`.
- Speechless / Unpleasant Stare → expression `speechless`, mood `speechless`.
- Scared → expression `scared`, mood `scared`.
- Hurt / Sad → expression `wounded`, mood `wounded`.
- Frightened → expression `frightened`, mood `frightened`.
