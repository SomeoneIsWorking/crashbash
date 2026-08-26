---
id: 11
title: Crash Bash projection anchors lack runtime submitter and camera-state attribution
status: investigating
symptom: The verified generated substrate contains projection/control sites, but no PC-owned producer can yet identify which retail camera and submitter state produced a visible frame
state_items: S004,S005,S006
tags: graphics,re,camera,native-renderer,widescreen,interpolation
created: 2026-08-26
updated: 2026-08-26
---

## Finding

`tools/inventory_render_anchors.py` now refuses unbound generated caches. Exact recorded pin
previous pin `17981527` has 1,724 generated functions and substrate baseline `99a42aa3` has 2,005;
both contain
31 projection anchors and 17 camera-control anchors, while three individual addresses differ because
the recompiler changed function boundaries. The strongest resident chain remains
`0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}` in both. This bounds a static RE slice but does
not prove runtime execution, camera semantics, primitive submission, or a native producer.

## Exact-pin runtime attempt

The operator's clean `99a42aa3` `rtpcaller` run reached resident, BOOT, and MENU code and printed
`empty prims`, but presented zero game frames before the 30-second watchdog exited 134. The watchdog
backtrace includes `gen_func_8002DE2C`, `ov_boot_gen_8008E5BC`, and
`ov_menu_gen_800B5218`; it does not include a completed 50-present `rtpcaller` histogram.
`tools/verify_render_anchor_reach.py` therefore correctly refused the exact local log at
`scratch/logs/crashbash-render-anchor-reach.log` because `[watchdog] STUCK` is forbidden. This is a
live falsifier of the current first-frame/reachability gate, not projection-anchor attribution.

## Proper resolution

First use the process-driving CDC gate with `--port`, `--executable`, and `--timeout` to identify the
post-MENU no-present cause; do not pass it a positional log (only `--trace PATH` judges a saved CDC
trace). After issues 0007/0009 pass that clean serialized CDC/completion gate, run the product with
`PSXPORT_DEBUG=rtpcaller` through at least 50 presented frames and feed the log to
`tools/verify_render_anchor_reach.py`. The judge binds every decoded projection target to the exact
generated inventory and refuses zero/unknown ancestry; its 8/8 selftest proves those opposite
answers. Decompile the observed ancestry back to decoded game camera/object/material state. Implement
native producers from that state, first proving 4:3 parity. Then identify two simulation snapshots
for interpolation and widen the native projection without changing vertical framing. Replaying or
patching GTE, OT, or GP0 output is explicitly rejected.
