#!/usr/bin/env python3
"""Convert spectral-prescreen JSONL into exact rho certificate records.

The C++ prescreen writes one ``spectral_candidate`` object per graph and
depth. Non-bipartite candidates are certified algebraically. Bipartite inputs
are archived but deliberately deferred until the parity-specific asymptotic
formula is implemented.
"""

import argparse
import json
import logging
import math
import subprocess
import sys
from pathlib import Path
from typing import Iterable, TextIO

from exact_rho import CertificationUnresolved, certify_rho_sign
from verify_hom import normalize_graph6, parse_graph6

SCHEMA = "exact-rho-sweep-record-v1"
CERTIFIED_STATUSES = {"certified_above_one", "certified_below_one"}
OPTIONAL_ARB_STATUSES = {
    "deferred_bipartite_parity",
    "invalid_disconnected",
    "unresolved",
    "unresolved_power_iteration",
    "invalid_enclosure",
}
VALID_PRECISIONS = {64, 128, 256, 512}
LOGGER = logging.getLogger(__name__)


def is_bipartite(adjacency: list[list[int]]) -> bool:
    """Return whether an undirected adjacency matrix is bipartite."""
    colors: list[int | None] = [None] * len(adjacency)
    for root in range(len(adjacency)):
        if colors[root] is not None:
            continue
        colors[root] = 0
        stack = [root]
        while stack:
            vertex = stack.pop()
            for neighbor, edge in enumerate(adjacency[vertex]):
                if not edge:
                    continue
                if neighbor == vertex:
                    return False
                if colors[neighbor] is None:
                    colors[neighbor] = 1 - colors[vertex]
                    stack.append(neighbor)
                elif colors[neighbor] == colors[vertex]:
                    return False
    return True


def run_arb_certifier(graph6: str, depth: int, executable: Path) -> dict:
    """Run the certified C++ backend and validate its JSON response."""
    executable = executable.expanduser().resolve()
    if not executable.is_file():
        raise RuntimeError(f"Arb certifier does not exist: {executable}")
    result = subprocess.run(
        # Arguments are passed directly with no shell interpretation.
        [str(executable), "--g6", graph6, "--depth", str(depth)],
        check=False,
        capture_output=True,
        text=True,
        shell=False,
    )
    try:
        certificate = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        detail = result.stderr.strip() or "no diagnostic"
        raise RuntimeError(f"Arb certifier returned invalid JSON: {detail}") from exc
    if certificate.get("schema") != "arb-rho-certificate-v1":
        raise RuntimeError("Arb certifier returned an unknown schema")
    if certificate.get("depth") != depth:
        raise RuntimeError("Arb certifier returned the wrong depth")
    status = certificate.get("status")
    valid_statuses = CERTIFIED_STATUSES | OPTIONAL_ARB_STATUSES
    if status not in valid_statuses:
        raise RuntimeError(f"Arb certifier returned invalid status {status!r}")
    if status in CERTIFIED_STATUSES:
        precision_bits = certificate.get("precision_bits")
        rho_interval = certificate.get("rho_interval")
        if (
            not isinstance(precision_bits, int)
            or precision_bits not in VALID_PRECISIONS
        ):
            raise RuntimeError("Arb certifier returned an invalid precision_bits value")
        if (
            not isinstance(rho_interval, str)
            or not rho_interval
            or rho_interval == "indeterminate"
            or not (rho_interval.startswith("[") and rho_interval.endswith("]"))
        ):
            raise RuntimeError("Arb certifier returned an invalid rho_interval value")
    if result.returncode not in (0, 1):
        raise RuntimeError(f"Arb certifier failed with exit code {result.returncode}")
    return certificate


def certify_candidate(candidate: dict, arb_certifier: Path | None = None) -> dict:
    """Validate and certify one C++ spectral-prescreen candidate."""
    if candidate.get("type") != "spectral_candidate":
        raise ValueError("input type must be spectral_candidate")
    graph6 = candidate.get("g6")
    depth = candidate.get("d")
    estimate = candidate.get("rho_estimate")
    if not isinstance(graph6, str) or not graph6.strip():
        raise ValueError("candidate must contain a nonempty g6 string")
    if not isinstance(depth, int) or depth < 1:
        raise ValueError("candidate depth must be a positive integer")
    if not isinstance(estimate, (int, float)) or not math.isfinite(estimate):
        raise ValueError("candidate rho_estimate must be finite")

    graph6 = normalize_graph6(graph6)
    adjacency = parse_graph6(graph6)
    record = {
        "schema": SCHEMA,
        "g6": graph6.strip(),
        "vertices": len(adjacency),
        "depth": depth,
        "rho_estimate": estimate,
    }
    if arb_certifier is not None:
        arb_certificate = run_arb_certifier(graph6.strip(), depth, arb_certifier)
        arb_status = arb_certificate["status"]
        record["arb_certificate"] = arb_certificate
        if arb_status in {"deferred_bipartite_parity", "invalid_disconnected"}:
            record["status"] = arb_status
            return record
        if arb_status in {"certified_above_one", "certified_below_one"}:
            record["status"] = arb_status
            return record

    if is_bipartite(adjacency):
        record["status"] = "deferred_bipartite_parity"
        return record

    try:
        certificate = certify_rho_sign(adjacency, [1] * len(adjacency), depth)
    except (CertificationUnresolved, RuntimeError) as exc:
        LOGGER.warning("Exact rho certification unresolved for %s: %s", graph6, exc)
        record["status"] = "unresolved"
        record["certificate_error"] = str(exc)
        return record
    record["status"] = {
        -1: "exact_below_one",
        0: "exact_equal_one",
        1: "exact_above_one",
    }[certificate.sign]
    record["certificate"] = certificate.as_dict()
    return record


def process_stream(
    source: Iterable[str], destination: TextIO, arb_certifier: Path | None = None
) -> int:
    """Certify candidate JSONL records and return the processed count."""
    count = 0
    for line_number, line in enumerate(source, 1):
        if not line.strip():
            continue
        try:
            candidate = json.loads(line)
            record = certify_candidate(candidate, arb_certifier)
        except (
            json.JSONDecodeError,
            KeyError,
            TypeError,
            ValueError,
            RuntimeError,
        ) as exc:
            raise ValueError(f"invalid candidate on line {line_number}: {exc}") from exc
        destination.write(json.dumps(record, sort_keys=True, separators=(",", ":")))
        destination.write("\n")
        count += 1
    return count


def main() -> None:
    """Read candidate JSONL and write deterministic certificate NDJSON."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="candidate JSONL; default stdin")
    parser.add_argument(
        "--output", type=Path, help="certificate NDJSON; default stdout"
    )
    parser.add_argument(
        "--arb-certifier",
        type=Path,
        help="certify with this C++ binary before exact symbolic fallback",
    )
    args = parser.parse_args()

    source = sys.stdin
    destination = sys.stdout
    try:
        if args.input is not None and args.output is not None:
            input_path = args.input.expanduser().resolve()
            output_path = args.output.expanduser().resolve()
            if input_path == output_path:
                raise ValueError("--input and --output must name different files")
        if args.input:
            source = args.input.open(encoding="utf-8")
        if args.output:
            destination = args.output.open("w", encoding="utf-8")
        process_stream(source, destination, args.arb_certifier)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(2) from exc
    finally:
        if args.input:
            source.close()
        if args.output:
            destination.close()


if __name__ == "__main__":
    main()
