#!/usr/bin/env python3
"""Build and launch the current Crash Bash port target from verified retail media."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "scratch/build-clang"
PORT = ROOT / "scratch/bin/crashbash_port"
EXE = ROOT / "scratch/bin/crashbash/SCUS_945.70"


class LaunchError(RuntimeError):
    """The product path could not be prepared."""


def command(arguments: list[str], environment: dict[str, str]) -> None:
    print("[run] " + " ".join(arguments), file=sys.stderr)
    result = subprocess.run(arguments, cwd=ROOT, env=environment, check=False)
    if result.returncode:
        raise LaunchError(f"command exited {result.returncode}: {' '.join(arguments)}")


def launch(disc: Path | None) -> None:
    environment = dict(os.environ)
    environment.update(CC="clang", CXX="clang++", CCACHE_DISABLE="1")
    if disc is not None:
        environment["PSXPORT_CRASHBASH_DISC"] = str(disc.resolve())
    jobs = min(os.cpu_count() or 1, 16)
    configure = [
        "cmake",
        "-S",
        ".",
        "-B",
        str(BUILD),
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
    ]
    command([sys.executable, "-B", "tools/psxport_sync.py", "--auto"], environment)
    command(configure, environment)
    command(
        [
            "cmake",
            "--build",
            str(BUILD),
            "--target",
            "discdump",
            "crt0_extract",
            "-j",
            str(jobs),
        ],
        environment,
    )
    provision = [sys.executable, "-B", "tools/provision.py"]
    if disc is not None:
        provision.append(str(disc))
    command(provision, environment)
    command(
        [
            sys.executable,
            "-B",
            "tools/recomp_bootstrap.py",
            "--ensure",
            "--crt0-extract",
            str(BUILD / "psxport_build/tools/crt0_extract"),
        ],
        environment,
    )
    command(
        configure, environment
    )  # generated/ now exists, so configure the product target
    command(
        ["cmake", "--build", str(BUILD), "--target", "crashbash_port", "-j", str(jobs)],
        environment,
    )
    if not PORT.is_file() or not EXE.is_file():
        raise LaunchError(
            f"build did not produce the required product inputs: {PORT}, {EXE}"
        )
    environment.setdefault("PSXPORT_ASSET_DIR", str(ROOT / "external/psxport"))
    print(f"[run] launching Crash Bash port: {PORT}", file=sys.stderr)
    os.execve(PORT, [str(PORT), str(EXE)], environment)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "disc",
        nargs="?",
        type=Path,
        help="optional Crash Bash USA CHD; otherwise use env/.env/drop-in",
    )
    args = parser.parse_args()
    try:
        launch(args.disc)
    except (OSError, LaunchError) as error:
        print(f"[run] error: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
