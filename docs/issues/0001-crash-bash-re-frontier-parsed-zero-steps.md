---
id: 1
title: Crash Bash RE frontier parsed zero steps
status: resolved
symptom: re_frontier.py reported zero ready and zero blocked because docs/re-frontier.md was an unstructured numbered list
tags: workflow,re
created: 2026-08-20
updated: 2026-08-20
---

## Root cause

The roadmap was an unstructured numbered list, while the shared frontier tracker indexes structured
heading/field entries. It therefore had no dependency graph to parse.

## What was tried / dead ends

Treating the zero-entry result as an empty roadmap would have hidden all seven known steps; the
tracker's refusal was correct and the document format was the defect.

## Resolution

### Resolution (2026-08-20)
Root cause: the roadmap did not use the tracker entry grammar, so no dependency graph existed. Replaced it with seven structured boot-to-graphics entries; check now parses 7 entries, identifies boot.provision as the only ready step, and reports 5 downstream steps blocked.
