#!/usr/bin/env python3
"""verify_hom.py — Exact-arithmetic Leontovich verifier.

Verifies homomorphism counts using Python's arbitrary-precision integers.
Confirms or dismisses anomalies flagged by leontovich_fast (which uses double).

Usage:
  python3 scripts/verify_hom.py NCQCCA?_B?K?W?g?K??
  echo 'NCQCCA?_B?K?W?g?K??' | python3 scripts/verify_hom.py -
"""

import sys

MAX_N = 202
MAX_D = 20


def parse_graph6(g6: str) -> list[list[int]]:
    """Parse a graph6 string into an adjacency matrix (integer entries)."""
    s = g6.strip()
    if s.startswith(">>graph6<<"):
        s = s[10:]
    m = ord(s[0]) - 63
    A = [[0] * m for _ in range(m)]
    k, bit_pos = 1, 5
    for col in range(1, m):
        for row in range(col):
            val = ord(s[k]) - 63
            if (val >> bit_pos) & 1:
                A[row][col] = 1
                A[col][row] = 1
            bit_pos -= 1
            if bit_pos < 0:
                k += 1
                bit_pos = 5
    return A


def compute_walks(A: list[list[int]], max_k: int) -> list[list[int]]:
    """Compute w[k] = A^k * 1 using exact integer arithmetic.

    w[k][i] = number of walks of length k starting at vertex i.
    """
    m = len(A)
    w = [[0] * m for _ in range(max_k)]
    # w[0] = all-ones vector
    w[0] = [1] * m

    for step in range(1, max_k):
        for i in range(m):
            s = 0
            for j in range(m):
                s += A[i][j] * w[step - 1][j]
            w[step][i] = s
    return w


def verify_graph(g6: str, verbose: bool = True) -> dict:
    """Verify a graph for Leontovich violations with exact arithmetic."""
    A = parse_graph6(g6)
    m = len(A)

    if verbose:
        print(f"Graph: {g6.strip()}")
        print(f"Vertices: {m}")

    # Compute walk vectors (exact integers)
    w = compute_walks(A, MAX_N)

    # homP[n] = hom(P_n, H) = sum_i w[n-1][i]
    homP = [0] * MAX_N
    homP[1] = m
    for n in range(2, MAX_N):
        homP[n] = sum(w[n - 1])

    results = {"g6": g6.strip(), "m": m, "violations": [], "anomalies": []}

    # Check E_n^{(d)} for d = 2..MAX_D
    for d in range(2, MAX_D + 1):
        # b[i] = w[1][i] * w[d][i]
        b = [w[1][i] * w[d][i] for i in range(m)]

        limit = 200 - d - 2
        for stem in range(limit + 1):
            # homE = sum_i w[stem][i] * b[i]
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

                if kind == "leontovich":
                    results["violations"].append(entry)
                else:
                    results["anomalies"].append(entry)

                if verbose:
                    print(
                        f"  {kind.upper()}: n={n}, d={d}, "
                        f"homE={homE}, homP={homP[n]}, "
                        f"diff={diff}, rel={rel:.2e}"
                    )

    if verbose:
        nv = len(results["violations"])
        na = len(results["anomalies"])
        print(f"\nSummary: {nv} violations, {na} anomalies (d>2)")
        if nv == 0 and na == 0:
            print("✓ No violations — graph is NOT Leontovich (exact arithmetic)")
        elif nv > 0:
            print("✗ GENUINE VIOLATION DETECTED")
        else:
            print("⚠ Anomalies only (d>2) — not Leontovich violations")

    return results


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <graph6_string|->")
        print("  Use '-' to read from stdin (one g6 per line)")
        sys.exit(1)

    if sys.argv[1] == "-":
        for line in sys.stdin:
            line = line.strip()
            if line:
                verify_graph(line)
                print()
    else:
        verify_graph(sys.argv[1])


if __name__ == "__main__":
    main()
