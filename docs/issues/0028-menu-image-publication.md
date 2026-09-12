---
id: 28
title: Direct Crash Bash player reaches MENU without authenticated image publication
status: investigating
symptom: BOOT continues after the 16-sector MENU read, then strict Lightrec dispatch refuses MENU entry 0x800B5244
state_items: S002,S003,S015
tags: menu,dynarec,loaded-image,authentication
created: 2026-09-12
updated: 2026-09-12
---

## Evidence and required transition

On pinned psxport `8b210329`, the direct player authenticated BOOT generation 3 and reached native
frames 0 and 1. The 16-sector MENU read into `0x800B32B4` partially replaced BOOT; image-range
subtraction preserved the still-executing BOOT code through the previous fault PC `0x80093244`.
After 6,390 guest cycles the next strict dispatch refused MENU entry `0x800B5244` because no
authenticated MENU generation covered it. Issue 0027 records the completed BOOT transition.

Recover the complete MENU read tuple and entry from the authenticated retail image and tracked
manifest, then validate the finished guest-RAM bytes before publishing the MENU image generation.
The publisher must preserve untouched BOOT fragments, invalidate overwritten translations, and
bind only MENU-native owners that belong to the new generation. A changed digest or wrong read
tuple must refuse; a bounded retail rerun must cross `0x800B5244` through Lightrec with explicit
translated and fallback denominators before any later gameplay claim.
