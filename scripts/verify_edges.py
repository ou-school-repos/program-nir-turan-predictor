#!/usr/bin/env python3
"""Exact-integer Leontovich verification directly from an edge list (loops OK).

Bypasses graph6 entirely -- graph6 has no diagonal bit, so any self-loop
edge [i, i] gets silently dropped on a round trip through it.

Usage: edit `edges` and `m` below, then run.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from verify_hom import MAX_D, MAX_N, compute_walks  # noqa: E402


def verify_edges(edges: list[list[int]], m: int, verbose: bool = True) -> dict:
    A = [[0] * m for _ in range(m)]
    for u, v in edges:
        if u == v:
            A[u][u] = 1
        else:
            A[u][v] = A[v][u] = 1

    if verbose:
        print(f"Vertices: {m}, edges (incl. loops): {edges}")

    w = compute_walks(A, MAX_N)

    homP = [0] * MAX_N
    homP[1] = m
    for n in range(2, MAX_N):
        homP[n] = sum(w[n - 1])

    results = {"m": m, "violations": [], "anomalies": []}

    for d in range(2, MAX_D + 1):
        b = [w[1][i] * w[d][i] for i in range(m)]
        limit = 200 - d - 2
        for stem in range(limit + 1):
            homE = sum(w[stem][i] * b[i] for i in range(m))
            n = stem + d + 2

            if homE < homP[n]:
                kind = "leontovich" if d == 2 else "anomaly"
                diff = homP[n] - homE
                rel = diff / homP[n] if homP[n] > 0 else 0

                entry = {
                    "type": kind,
                    "n": n,
                    "d": d,
                    "homE": homE,
                    "homP": homP[n],
                    "diff": diff,
                    "rel_diff": rel,
                }
                (
                    results["violations"]
                    if kind == "leontovich"
                    else results["anomalies"]
                ).append(entry)

                if verbose:
                    print(
                        f"  {kind.upper()}: n={n}, d={d}, "
                        f"homE={homE}, homP={homP[n]}, diff={diff}, rel={rel:.2e}"
                    )

    if verbose:
        nv, na = len(results["violations"]), len(results["anomalies"])
        print(f"\nSummary: {nv} violations, {na} anomalies (d>2)")
        if nv == 0 and na == 0:
            print("No violations -- graph is NOT Leontovich (exact arithmetic)")
        elif nv > 0:
            print("GENUINE VIOLATION DETECTED (Leontovich confirmed)")
        else:
            print("Anomalies only (d>2) -- not a Leontovich violation")

    return results


def main():
    edges = [
        [0, 1],
        [0, 2],
        [0, 3],
        [0, 5],
        [0, 7],
        [0, 9],
        [0, 12],
        [0, 15],
        [1, 3],
        [1, 5],
        [1, 7],
        [1, 11],
        [1, 13],
        [2, 2],
        [2, 6],
        [2, 12],
        [2, 14],
        [3, 7],
        [3, 15],
        [4, 5],
        [4, 6],
        [4, 10],
        [5, 9],
        [5, 12],
        [6, 12],
        [6, 14],
        [7, 8],
        [7, 9],
        [7, 10],
        [8, 8],
        [8, 12],
        [8, 13],
        [9, 9],
        [9, 10],
        [9, 14],
        [10, 11],
        [10, 14],
        [11, 12],
        [11, 14],
        [11, 15],
        [13, 13],
        [13, 15],
    ]
    m = 16
    verify_edges(edges, m)


if __name__ == "__main__":
    main()
