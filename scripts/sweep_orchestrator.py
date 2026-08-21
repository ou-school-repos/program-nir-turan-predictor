#!/usr/bin/env python3
"""Convert spectral-prescreen JSONL into exact rho certificate records.

The C++ prescreen writes one ``spectral_candidate`` object per graph and
depth. Non-bipartite candidates are certified algebraically. Bipartite inputs
are archived but deliberately deferred until the parity-specific asymptotic
formula is implemented.
"""

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Iterable, TextIO

from exact_rho import certify_rho_sign
from verify_hom import parse_graph6

SCHEMA = "exact-rho-sweep-record-v1"


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


def certify_candidate(candidate: dict) -> dict:
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

    adjacency = parse_graph6(graph6)
    record = {
        "schema": SCHEMA,
        "g6": graph6.strip(),
        "vertices": len(adjacency),
        "depth": depth,
        "rho_estimate": estimate,
    }
    if is_bipartite(adjacency):
        record["status"] = "deferred_bipartite_parity"
        return record

    certificate = certify_rho_sign(adjacency, [1] * len(adjacency), depth)
    record["status"] = {
        -1: "exact_below_one",
        0: "exact_equal_one",
        1: "exact_above_one",
    }[certificate.sign]
    record["certificate"] = certificate.as_dict()
    return record


def process_stream(source: Iterable[str], destination: TextIO) -> int:
    """Certify candidate JSONL records and return the processed count."""
    count = 0
    for line_number, line in enumerate(source, 1):
        if not line.strip():
            continue
        try:
            candidate = json.loads(line)
            record = certify_candidate(candidate)
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
    args = parser.parse_args()

    source = args.input.open(encoding="utf-8") if args.input else sys.stdin
    destination = args.output.open("w", encoding="utf-8") if args.output else sys.stdout
    try:
        process_stream(source, destination)
    finally:
        if args.input:
            source.close()
        if args.output:
            destination.close()


if __name__ == "__main__":
    main()
