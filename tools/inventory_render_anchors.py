#!/usr/bin/env python3
"""Inventory retail-derived projection anchors in generated recompilation sources.

This tool reads generated code but never modifies it.  A projection anchor is a
generated function that both executes a GTE operation and consumes projected
screen/depth output.  That is a narrow static fact: it identifies RE entry points,
not a native renderer and not proof that a function submitted a visible primitive.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path

import recomp_bootstrap

FUNCTION_RE = re.compile(
    r"^void (?:(ov_boot|ov_menu)_)?(?:gen_func_|gen_)([0-9A-F]{8})\(Core\* c\) \{",
    re.MULTILINE,
)
CALL_RE = re.compile(r"(?:(ov_boot|ov_menu)_)?func_([0-9A-F]{8})\(c\)")
GTE_OP_RE = re.compile(r"gte_op_at\(c, (0x[0-9A-F]+)u, (0x[0-9A-F]+)u\)")
GTE_DEPTH_RE = re.compile(r"gte_read_data\((?:17|18|19)\)")
GTE_CTRL_RE = re.compile(r"gte_write_ctrl\((\d+),")


@dataclass(frozen=True)
class GeneratedFunction:
    address: str
    module: str
    source: str
    body: str

    @property
    def callees(self) -> tuple[tuple[str, str], ...]:
        def module_for_prefix(prefix: str) -> str:
            return {"ov_boot": "BOOT", "ov_menu": "MENU", "": "resident"}[prefix]

        return tuple(
            sorted(
                (module_for_prefix(prefix), address)
                for prefix, address in set(CALL_RE.findall(self.body))
            )
        )

    @property
    def gte_operations(self) -> tuple[tuple[str, str], ...]:
        return tuple(GTE_OP_RE.findall(self.body))

    @property
    def screen_stores(self) -> int:
        return self.body.count("gte_store_xy(")

    @property
    def depth_reads(self) -> int:
        return len(GTE_DEPTH_RE.findall(self.body))

    @property
    def packet_writes(self) -> int:
        return self.body.count("mem_w32(") + self.body.count("mem_w16(")

    @property
    def control_registers(self) -> tuple[int, ...]:
        return tuple(
            sorted({int(register) for register in GTE_CTRL_RE.findall(self.body)})
        )

    @property
    def is_projection_anchor(self) -> bool:
        return bool(self.gte_operations) and bool(
            self.screen_stores or self.depth_reads
        )

    @property
    def is_camera_control_anchor(self) -> bool:
        camera_registers = {*range(8), 24, 25, 26}
        return bool(camera_registers.intersection(self.control_registers))


@dataclass(frozen=True)
class GeneratedProvenance:
    psxport_commit: str
    recompiler_version: str
    roots: int
    functions: int
    output_sha256: str

    def as_dict(self) -> dict[str, object]:
        return {
            "psxport_commit": self.psxport_commit,
            "recompiler_version": self.recompiler_version,
            "roots": self.roots,
            "functions": self.functions,
            "output_sha256": self.output_sha256,
        }


def validate_generated_identity(
    *,
    generated_version: str,
    measured_version: str,
    selected_version: str,
    generated_input_hash: str,
    expected_input_hash: str,
    actual_output_hash: str,
    measured_output_hash: str,
) -> None:
    if generated_version != measured_version:
        raise ValueError(
            "generated version stamp disagrees with its measurement: "
            f"{generated_version!r} != {measured_version!r}"
        )
    if generated_version != selected_version:
        raise ValueError(
            "generated cache belongs to recompiler version "
            f"{generated_version}, selected psxport provides {selected_version}; "
            "run tools/recomp_bootstrap.py --ensure with the intended PSXPORT_DIR"
        )
    if generated_input_hash != expected_input_hash:
        raise ValueError(
            "generated input hash does not match the selected psxport and retail inputs; "
            "run tools/recomp_bootstrap.py --ensure before inventorying"
        )
    if actual_output_hash != measured_output_hash:
        raise ValueError(
            "generated output hash disagrees with its measurement: "
            f"{actual_output_hash} != {measured_output_hash}"
        )


def module_for_path(path: Path) -> str:
    name = path.name
    if name.startswith("ov_boot_"):
        return "BOOT"
    if name.startswith("ov_menu_"):
        return "MENU"
    return "resident"


def parse_functions(text: str, module: str, source: str) -> list[GeneratedFunction]:
    matches = list(FUNCTION_RE.finditer(text))
    functions: list[GeneratedFunction] = []
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        functions.append(
            GeneratedFunction(
                address=match.group(2),
                module=module,
                source=source,
                body=text[match.start() : end],
            )
        )
    return functions


def source_paths(generated: Path) -> list[Path]:
    paths = [*generated.glob("shard_[0-9]*.c"), *generated.glob("ov_*_shard_[0-9]*.c")]
    return sorted(path for path in paths if path.is_file())


def load_functions(generated: Path) -> list[GeneratedFunction]:
    paths = source_paths(generated)
    if not paths:
        raise ValueError(f"no generated shard sources found under {generated}")
    functions: list[GeneratedFunction] = []
    for path in paths:
        functions.extend(
            parse_functions(
                path.read_text(encoding="utf-8"),
                module_for_path(path),
                path.name,
            )
        )
    return functions


def load_provenance(generated: Path) -> GeneratedProvenance:
    measurement_path = generated / ".recomp.measurement.json"
    version_path = generated / ".recomp_version"
    hash_path = generated / ".recomp.hash"
    try:
        measurement = json.loads(measurement_path.read_text(encoding="utf-8"))
        generated_version = version_path.read_text(encoding="utf-8").strip()
        generated_input_hash = hash_path.read_text(encoding="utf-8").strip()
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read generated provenance: {error}") from error
    if not isinstance(measurement, dict):
        raise TypeError(f"generated measurement {measurement_path} is not an object")
    try:
        roots = int(measurement["roots"])
        functions = int(measurement["functions"])
        measured_version = str(measurement["version"])
        output_sha256 = str(measurement["output_sha256"])
    except (KeyError, TypeError, ValueError) as error:
        raise ValueError(
            f"generated measurement {measurement_path} is incomplete"
        ) from error
    emitter = recomp_bootstrap.recompiler_sources()[0]
    try:
        emitter_text = emitter.read_text(encoding="utf-8")
    except OSError as error:
        raise ValueError(
            f"cannot read selected recompiler {emitter}: {error}"
        ) from error
    version_match = re.search(
        r'^RECOMP_VERSION\s*=\s*"([^"]+)"', emitter_text, re.MULTILINE
    )
    if version_match is None:
        raise ValueError(f"selected recompiler {emitter} has no RECOMP_VERSION")
    selected_version = version_match.group(1)
    expected_input_hash = f"{selected_version}:{recomp_bootstrap.input_hash()}"
    actual_output_hash = recomp_bootstrap.generated_output_hash(generated)
    validate_generated_identity(
        generated_version=generated_version,
        measured_version=measured_version,
        selected_version=selected_version,
        generated_input_hash=generated_input_hash,
        expected_input_hash=expected_input_hash,
        actual_output_hash=actual_output_hash,
        measured_output_hash=output_sha256,
    )
    try:
        psxport_commit = subprocess.run(
            ["git", "-C", str(recomp_bootstrap.psxport_dir()), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    except subprocess.CalledProcessError as error:
        raise ValueError("cannot resolve selected psxport commit") from error
    if not re.fullmatch(r"[0-9a-f]{40}", psxport_commit):
        raise ValueError(f"selected psxport commit is invalid: {psxport_commit!r}")
    return GeneratedProvenance(
        psxport_commit=psxport_commit,
        recompiler_version=generated_version,
        roots=roots,
        functions=functions,
        output_sha256=output_sha256,
    )


def build_inventory(
    functions: Iterable[GeneratedFunction],
    provenance: GeneratedProvenance | None = None,
) -> dict[str, object]:
    all_functions = list(functions)
    callers: dict[tuple[str, str], set[tuple[str, str]]] = {}
    for function in all_functions:
        for callee in function.callees:
            callers.setdefault(callee, set()).add((function.module, function.address))

    anchors = [function for function in all_functions if function.is_projection_anchor]
    anchors.sort(
        key=lambda function: (
            -len(function.gte_operations),
            -function.screen_stores,
            -function.depth_reads,
            -function.packet_writes,
            function.module,
            function.address,
        )
    )

    camera_anchors = [
        function for function in all_functions if function.is_camera_control_anchor
    ]
    camera_anchors.sort(
        key=lambda function: (
            -len(set(function.control_registers).intersection({24, 25, 26})),
            -len(set(function.control_registers).intersection(range(8))),
            function.module,
            function.address,
        )
    )

    def direct_callers(function: GeneratedFunction) -> list[dict[str, str]]:
        return [
            {"address": f"0x{address}", "module": module}
            for module, address in sorted(
                callers.get((function.module, function.address), set())
            )
        ]

    inventory: dict[str, object] = {
        "schema": 1,
        "generated_functions": len(all_functions),
        "projection_anchors": len(anchors),
        "camera_control_anchors": len(camera_anchors),
        "modules": {
            module: {
                "functions": sum(
                    function.module == module for function in all_functions
                ),
                "projection_anchors": sum(
                    function.module == module for function in anchors
                ),
                "camera_control_anchors": sum(
                    function.module == module for function in camera_anchors
                ),
            }
            for module in ("resident", "BOOT", "MENU")
        },
        "anchors": [
            {
                "address": f"0x{function.address}",
                "module": function.module,
                "source": function.source,
                "gte_operations": len(function.gte_operations),
                "gte_pcs": [pc for _opcode, pc in function.gte_operations],
                "screen_stores": function.screen_stores,
                "depth_reads": function.depth_reads,
                "packet_writes": function.packet_writes,
                "control_registers": list(function.control_registers),
                "direct_callers": direct_callers(function),
            }
            for function in anchors
        ],
        "camera_anchors": [
            {
                "address": f"0x{function.address}",
                "module": function.module,
                "source": function.source,
                "control_registers": list(function.control_registers),
                "projection_output": function.is_projection_anchor,
                "direct_callers": direct_callers(function),
            }
            for function in camera_anchors
        ],
    }
    if provenance is not None:
        if len(all_functions) != provenance.functions:
            raise ValueError(
                "parsed function denominator disagrees with generated measurement: "
                f"{len(all_functions)} != {provenance.functions}"
            )
        inventory["provenance"] = provenance.as_dict()
    return inventory


def run_selftest() -> None:
    fixture = """
void gen_func_80001000(Core* c) {
  gte_write_ctrl(26, c->r[4]);
  gte_op_at(c, 0x4A280030u, 0x80001020u);
  gte_store_xy(c, c->r[4], 14);
  c->mem_w32(c->r[5], gte_read_data(19));
}
void gen_func_80002000(Core* c) {
  func_80001000(c);
}
void gen_func_80003000(Core* c) {
  gte_op_at(c, 0x4A00412u, 0x80003010u);
}
void gen_func_80004000(Core* c) {
  c->mem_w32(c->r[5], c->r[2]);
}
"""
    inventory = build_inventory(parse_functions(fixture, "resident", "fixture.c"))
    anchors = inventory["anchors"]
    assert inventory["generated_functions"] == 4
    assert inventory["projection_anchors"] == 1
    assert inventory["camera_control_anchors"] == 1
    assert isinstance(anchors, list)
    assert anchors[0]["address"] == "0x80001000"
    assert anchors[0]["screen_stores"] == 1
    assert anchors[0]["depth_reads"] == 1
    assert anchors[0]["direct_callers"] == [
        {"address": "0x80002000", "module": "resident"}
    ]
    assert anchors[0]["control_registers"] == [26]
    validate_generated_identity(
        generated_version="v1",
        measured_version="v1",
        selected_version="v1",
        generated_input_hash="v1:input",
        expected_input_hash="v1:input",
        actual_output_hash="output",
        measured_output_hash="output",
    )
    try:
        validate_generated_identity(
            generated_version="v1",
            measured_version="v1",
            selected_version="v2",
            generated_input_hash="v1:input",
            expected_input_hash="v2:input",
            actual_output_hash="output",
            measured_output_hash="output",
        )
    except ValueError:
        pass
    else:
        raise AssertionError("a cache from another selected recompiler was accepted")
    try:
        build_inventory(
            parse_functions(fixture, "resident", "fixture.c"),
            GeneratedProvenance("0" * 40, "v1", 1, 5, "output"),
        )
    except ValueError:
        pass
    else:
        raise AssertionError("a mismatched function denominator was accepted")
    print("render-anchor inventory selftest: 12/12 checks passed")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--generated",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "generated",
        help="generated recompilation source directory",
    )
    parser.add_argument(
        "--selftest",
        action="store_true",
        help="exercise positive and negative fixtures",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.selftest:
        run_selftest()
        return 0
    try:
        provenance = load_provenance(args.generated)
        inventory = build_inventory(load_functions(args.generated), provenance)
    except (OSError, TypeError, ValueError, recomp_bootstrap.Refused) as error:
        print(f"render-anchor inventory: REFUSED: {error}", file=sys.stderr)
        return 2
    print(json.dumps(inventory, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
