---
id: C014
kind: claim
status: holds
created: 2026-08-24
tags:
depends: bootstrap.py#main, bootstrap.py#prepare_product, run.sh, CMakeLists.txt
---

## Claim

Crash Bash's player launcher uses the frozen uv environment, forwards that exact interpreter to CMake and project tools, keeps zero arguments bound to crashbash_port plus SCUS_945.70, and applies no compiler identity whitelist or blacklist

## Evidence

tests/test_bootstrap.py passed 11/11 hermetic launcher controls inside uv.lock and the full Python suite passed 23/23. A real `bootstrap.py --prepare-only` run against clean psxport 9c2e3f1c resolved Python3 to `.venv/bin/python3`, verified the retail executable 11/11 and loaded modules 14/14, retained the 1,063-root/1,724-function substrate, and linked `crashbash_port` from the isolated `scratch/build/player` tree. Its cache records `BUILD_TESTING=OFF` and `PSXPORT_BUILD_TESTS=OFF`, with no CTest registration. A separate Clang 22 maintainer build passed 8/8 CTest. No game binary or window was launched in this verification.

## What would falsify it

If run.sh stops using uv --frozen, a child Python/CMake invocation escapes sys.executable, zero arguments select another target or omit the required executable, compiler identity policy returns, or either non-launching mode performs setup/exec
