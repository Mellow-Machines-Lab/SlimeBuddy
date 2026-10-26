# SlimeBuddy Agent Instructions

These instructions apply to the whole repository. `CLAUDE.md` and
`AGENTS.md` are the same file: edit `CLAUDE.md`, then copy it over
`AGENTS.md`.

## What this is

SlimeBuddy is the gel slime, one character for Boop, the desk creature that watches
your Claude Code and Codex agents. It runs on the Waveshare ESP32-S3-Touch-AMOLED-2.06. Boop itself, the
engine every character runs on, is
[Mellow-Machines-Lab/buddygotchi](https://github.com/Mellow-Machines-Lab/buddygotchi),
here as the `engine/` submodule. Its `CLAUDE.md` has the engine's rules,
words and architecture, and they apply here too.

| Path | What it is |
| --- | --- |
| `character/` | The character pack: everything that makes this character this one. The contract it meets is the engine's [characters/CHARACTER.md](https://github.com/Mellow-Machines-Lab/buddygotchi/blob/main/characters/CHARACTER.md) |
| `engine/` | Boop's repository, a submodule, pinned to the commit this character is built and tested with |
| `media/` | The README's pictures (`make media`) and the social preview |
| `Makefile` | Every command, each the engine's with this pack |
| `.github/workflows/` | CI: the pack's checks, the Swift tests, the firmware's tests and this board's firmware |

## What goes where

- **Only what is this character's goes here**: its `character.json`,
  personality and steering, moods, faces' art and code
  (`character/firmware/`), sounds and voice, rig, demos, evals, its
  tests and the tools that make its files.
- **Everything every character shares goes in the engine**: the Mac app,
  the firmware's core, the device protocol, the brain's harness, the
  studio, `charactergen.py`, the dev tools, and anything another
  character could use. Make that change in buddygotchi first, then bump
  `engine/` here.
- A change that spans both lands in the engine first, so this repository
  never pins an engine commit that isn't pushed.
- Every repository squash-merges, so each change is one commit.
- Only the owner pushes, tags or force-pushes, or says to.

## Setting up

A fresh clone needs `git clone --recurse-submodules`; `make setup` (which
every target runs) checks the engine out if it's empty, links this pack
into it and chooses it. Any make target works from a clone straight away.

## Commands

```sh
make flash      # the firmware onto the board on USB
make bundle     # Boop.app with this character, here
make dev        # the debug app, simulator and studio (Bluetooth: the owner's)
make demo       # a demo: asks which and where (NAME=… plays one at once)
make check      # every check that needs no board, camera or Jev key
make eval       # the evals against Jev (needs BOOP_JEV_KEY)
make media      # the README's pictures again
```

The engine's own tools work as they do there, from `engine/`, with this
pack chosen: `cd engine && internal/tools/boopctl --help`. After changing
a demo's script, record it again and commit the recording
(`make -C engine/internal demos`, into `character/demo/recordings/`).

## Bumping the engine

```sh
git -C engine fetch && git -C engine checkout origin/main
git -C engine submodule update --init --recursive
make check
git add engine && git commit
```

The message says what came in: `engine: bump to <short sha>`, then each
new commit's subject (`git -C engine log --oneline OLD..NEW`).

To work on the engine from here, branch in `engine/` from its `main`, and
follow its `CLAUDE.md`: the change is a pull request there, then a bump
here. To push from here (the owner's, when asked), use the SSH alias that
can write to the organisation:

```sh
git remote set-url --push origin git@github-fdsa:Mellow-Machines-Lab/SlimeBuddy.git
git -C engine remote set-url --push origin git@github-fdsa:Mellow-Machines-Lab/buddygotchi.git
```

## Docs stay in sync

The pack's own docs are under `character/` (its `design/` and `tools/`
READMEs). A change that alters what one says updates it in the same
commit. The README describes the character and how to start; keep its
commands the Makefile's.
