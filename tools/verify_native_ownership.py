#!/usr/bin/env python3
"""Verify Crash Bash source ownership without duplicating the policy implementation."""

from __future__ import annotations

import argparse
from pathlib import Path

from source_policy import SourcePolicyError, check_source_policy


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        report = check_source_policy(args.root)
    except SourcePolicyError as error:
        print(f"Crash Bash source policy: FAIL: {error}")
        return 1
    print(
        "Crash Bash source policy: PASS: "
        f"scanned {report.source_files} source files, "
        f"{report.override_registrations} runtime override registrations, "
        f"{report.original_calls} scoped original calls"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
