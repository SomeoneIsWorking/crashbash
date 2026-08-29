---
id: 20
title: START reaches Crash Bash game input state but does not leave BOOT/attract flow
status: investigating
symptom: Fresh headless START A/B proves the guest SIO packet, parsed P1 word, and game-facing active-high state all change correctly, yet the process remains on the BOOT/attract state and no menu transition occurs.
state_items: S002
tags: pad,input,flow,attract,menu,re-frontier
created: 2026-08-30
updated: 2026-08-30
---

## Root cause

Unknown. Input delivery is no longer a candidate: a frame-200 idle/START A/B proves the
guest's SIO packet, parsed active-low word, and game-facing active-high P1 state all change at
the expected addresses. The earlier labels `0x80078C90` as an "app mode" and `0x8004E0B8` as
the corresponding state were wrong: `0x80078C90` is the BOOT vtable value stored at
`0x8004E0DC`, while `0x8004E0B8` is the initial process-state object stored at `0x8005B648`.
The next investigation must trace readers of the proven `0x8005133C == 8` state into the
active attract-process decision and find the first branch that differs from retail.

## What was tried / dead ends

- Feeding `padSlot0Buf` or `0x8007787C` is rejected: Crash Bash reads its own guest SIO
  pipeline, which issue 0019 now proves operational.
- Extending the run to 9000/40000 frames does not make held START transition on its own.
- Treating unchanged BOOT vtable/process-object pointers as a failed pad gate conflates two
  boundaries; the pad state changes correctly while those pointers remain stable.

## Resolution

Open. Gate the eventual fix at the first RE-grounded attract-flow state or branch downstream
of `0x8005133C`, not by expecting an unrelated vtable pointer to change.
