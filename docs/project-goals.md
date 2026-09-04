# Project goals

## Product architecture

Crash Bash is one native/dynarec hybrid product. Title-owned native overrides execute the behavior
the port deliberately owns; every remaining guest instruction executes on demand through psxport's
pinned Lightrec integration from the user's authenticated USA game image.

An interpreter may exist only in a separately built test target, including diagnostics. The gameplay product
must not link it, expose an execution-engine selector for it, or fall back to it. No build,
provisioning, installation, or launch path may emit or compile a static guest-code corpus.

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

Pixel-exact agreement is not a completion gate. The executable, assets, simulation, input, timing,
audio, and scene semantics remain authoritative; independent comparison is a diagnostic for finding
causes. Presentation is accepted by driving the running product through representative gameplay and
checking that it works and looks correct.

## G001 — A faithful, portable Crash Bash PC port

Crash Bash should run from a user-supplied verified USA disc through the default launcher, preserving
the retail game's observable simulation, input, audio, and presentation behavior.

Success conditions:

- `./run.sh` authenticates the user's game input, builds the intended product with a supported host
  compiler, and launches without maintainer-only RE tools or offline guest translation.
- psxport dynamically translates every non-native guest path through its pinned Lightrec revision.
- The gameplay link, configuration, and runtime surfaces contain no interpreter, interpreter selector,
  or interpreter fallback.
- The existing 27 native override installations remain active. All 15 calls from native owners to a
  generated guest body are replaced by psxport's scoped runtime original-call operation, which bypasses
  only the current override and executes the authenticated guest body through Lightrec.
- The static generator, generated corpus, dispatcher, seeds, and static-only checks are absent before
  dynarec implementation begins and cannot be restored as a bridge.
- Independent emulator, binary, or evidence from a separately built test target, including diagnostics,
  remains available to locate
  the first divergence without retaining the static product as an oracle.

Constraints and non-goals: copyrighted disc and module bytes remain untracked; boot, a logo, a menu,
or a single rendered frame is not representative-gameplay conformance; an interpreter-backed gameplay
mode or a compatibility static product is not shipped.

Contributing state items: S001-S004, S007-S017.

## G002 — Native widescreen presentation

Crash Bash should render from decoded game-owned camera, object, and material state through PC-owned
graphics producers, with a wider horizontal field of view and unchanged vertical framing.

Success conditions:

- The native 4:3 path looks correct at representative gameplay and UI boundaries before aspect-ratio
  divergence is accepted.
- Wider aspect ratios expand horizontal view without changing simulation, vertical framing, HUD intent,
  visibility policy, or guest RAM.
- Native output is judged on the running product; retail state and image comparison locate the cause of
  a visible defect rather than defining the pass mark.

Constraints and non-goals: GTE registers, ordering-table packets, GP0 output, and framebuffer pixels
may locate RE boundaries but are not native scene inputs.

Contributing state items: S004, S005, S007, S016.

## G003 — Smooth 60 Hz presentation without faster simulation

Crash Bash should present native camera and world motion at 60 Hz by interpolating between consecutive
retail simulation snapshots while leaving the simulation cadence and results unchanged.

Success conditions:

- Disabling interpolation is behaviorally identical to the native 4:3 path.
- Alpha 0 and alpha 1 reproduce the bounding simulation snapshots, and an alpha-0.5 render produces a
  verified midpoint without mutating simulation or guest RAM.
- Screen-space UI and other non-interpolated layers retain their intended cadence and placement.

Constraints and non-goals: projected screen coordinates, guest GTE matrices, packet streams, and
framebuffer images are not interpolation inputs; native scene snapshots own the temporal data.

Contributing state items: S004, S006, S007, S016.
