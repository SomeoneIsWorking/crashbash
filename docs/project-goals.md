# Project goals

## G001 — A faithful, portable Crash Bash PC port

Crash Bash should run from a user-supplied verified USA disc through the default launcher and retain
the retail game's observable simulation, input, audio, and presentation behavior.

Why it matters: the project exists to make the retail game natively runnable and maintainable on PC,
not to replace missing execution with a demo or title-specific shortcut.

Success conditions:

- `./run.sh` provisions restricted inputs outside git, builds the intended product with a supported
  host compiler, and launches it without maintainer-only RE tools.
- The complete retail execution spine reaches playable game modes without game-local BIOS, CD,
  loaded-module, or graphics fallbacks.
- Deterministic comparisons against an independent retail oracle cover every replaced boundary and
  retain a discoverable first divergence.

Constraints and non-goals: generated code remains derived and unedited; copyrighted disc/module
bytes remain untracked; a fabricated boot/gameplay picture or title-local hardware emulation is not
an acceptable substitute for faithful execution.

Contributing state items: S001, S002, S003, S004, S007.

## G002 — Native widescreen presentation

Crash Bash should render from decoded game-owned camera, object, and material state through PC-owned
graphics producers, with a wider horizontal field of view and unchanged vertical framing.

Why it matters: true widescreen requires ownership of the camera and scene semantics. Stretching the
retail framebuffer or patching projected guest coordinates does not expose more of the game world.

Success conditions:

- Native producers reproduce the retail 4:3 frame before any aspect-ratio divergence is enabled.
- Wider aspect ratios expand horizontal view without changing simulation, vertical framing, HUD
  intent, visibility policy, or guest RAM.
- Native output is verified against independent retail state and image evidence at representative
  gameplay and UI boundaries.

Constraints and non-goals: GTE registers, ordering-table packets, GP0 output, and framebuffer pixels
may locate RE boundaries but are never accepted as native scene input.

Contributing state items: S004, S005, S007.

## G003 — Smooth 60 Hz presentation without faster simulation

Crash Bash should present native camera and world motion at 60 Hz by interpolating between consecutive
retail simulation snapshots while leaving the simulation cadence and results unchanged.

Why it matters: doubling simulation speed changes game behavior; presentation interpolation provides
smooth motion without rewriting the retail timing model.

Success conditions:

- Disabling interpolation is behaviorally identical to the verified native 4:3 path.
- Alpha 0 and alpha 1 reproduce the bounding simulation snapshots, and an alpha-0.5 render produces
  a verified midpoint without mutating simulation or guest RAM.
- Screen-space UI and other non-interpolated layers retain their intended cadence and placement.

Constraints and non-goals: projected screen coordinates, guest GTE matrices, packet streams, and
framebuffer images are not interpolation inputs; native scene snapshots must own the temporal data.

Contributing state items: S004, S006, S007.
