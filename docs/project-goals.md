# Project goals

## The bar is a working game that looks correct

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

Pixel-exact agreement with the PSX reference is NOT a goal and is not a gate for any goal below. The
deliverable is a game that runs, plays, and looks right. Differential comparison against the retail
oracle remains a DIAGNOSTIC — the way to find out why something looks wrong or behaves wrong — never a
completion condition, and no goal is held open by a residual pixel count.

Correctness still means the retail game, not an approximation of it: simulation, input, timing, audio,
and scene semantics come from the real executable and real assets, and a fabricated picture or a
title-local shortcut is still not an acceptable substitute. What changed is the evidence bar for
presentation — visible correctness at gameplay and UI boundaries, judged on the running product.

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
- Every replaced boundary keeps a deterministic comparison against an independent retail oracle
  available as a diagnostic, so a wrong behavior can be traced to its cause rather than guessed at.

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

- The native 4:3 path looks correct — no missing, misplaced, mis-ordered, or wrongly colored layers
  at representative gameplay and UI boundaries — before any aspect-ratio divergence is enabled.
- Wider aspect ratios expand horizontal view without changing simulation, vertical framing, HUD
  intent, visibility policy, or guest RAM.
- Native output is judged on the running product; retail state and image comparison locate the cause
  of a visible defect rather than defining the pass mark.

Constraints and non-goals: GTE registers, ordering-table packets, GP0 output, and framebuffer pixels
may locate RE boundaries but are never accepted as native scene input.

Contributing state items: S004, S005, S007.

## G003 — Smooth 60 Hz presentation without faster simulation

Crash Bash should present native camera and world motion at 60 Hz by interpolating between consecutive
retail simulation snapshots while leaving the simulation cadence and results unchanged.

Why it matters: doubling simulation speed changes game behavior; presentation interpolation provides
smooth motion without rewriting the retail timing model.

Success conditions:

- Disabling interpolation is behaviorally identical to the native 4:3 path.
- Alpha 0 and alpha 1 reproduce the bounding simulation snapshots, and an alpha-0.5 render produces
  a verified midpoint without mutating simulation or guest RAM.
- Screen-space UI and other non-interpolated layers retain their intended cadence and placement.

Constraints and non-goals: projected screen coordinates, guest GTE matrices, packet streams, and
framebuffer images are not interpolation inputs; native scene snapshots must own the temporal data.

Contributing state items: S004, S006, S007.
