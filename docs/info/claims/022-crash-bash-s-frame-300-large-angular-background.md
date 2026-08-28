---
id: C022
kind: claim
status: holds
created: 2026-08-28
tags: graphics,native-renderer,depth
depends: game/render/model_depth_scale_capture.cpp, game/render/model_face_coverage.cpp
---

## Claim

Crash Bash's frame-300 large angular background holes were caused by fixed-model third-SZ depth classification plus unpublished title ZSF3, and exact AVSZ3/OTZ ownership closes them

## Evidence

Retail 0x800193A8 executes AVSZ3 and consumes OTZ; title initializer 0x80033494 writes CR29=341. Exact PID 4054917 at 138ad0a-dirty+psxport-ff21584d completed 301/301 frames: face261 packet/native SXY match, ZSF3=341, OTZ=1511, sort=1755, accepted and writes display (234,181); output (439,543) changed from black to (66,8,90) versus PSX (66,0,90), and full-frame inspection showed the broad holes closed with the logo intact.

## What would falsify it

if a current authentic frame-300 run no longer publishes ZSF3 341 from 0x80033494, face261 does not match retail projection or OTZ 1511, or the broad black holes recur with this owner active
