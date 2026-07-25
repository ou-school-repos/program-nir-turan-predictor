#!/usr/bin/env python3
"""Exact brute-force search for small Leontovich graphs (loops allowed).

Enumerates every labeled graph on m vertices (2^(m*(m+1)/2) adjacency
matrices, upper triangle incl. diagonal for loops), filters out
disconnected / degree-0 graphs cheaply, then runs exact-integer
homomorphism counting on the survivors. No Z3, no floats, no graph6.

Only practical up to m ~ 7-8 (2^28 / 2^36 matrices) -- use --shard/--shards
to split the range across multiple processes/cores for larger m.

Usage:
    python3 scripts/leontovich_brute.py -m 6
    python3 scripts/leontovich_brute.py -m 7 --shards 6 --shard 0   # run 6 of these, shard=0..5
"""

import argparse
import sys
import time


def slots_for(m):
    """Upper-triangle incl. diagonal: (i, j) for i <= j."""
    return [(i, j) for i in range(m) for j in range(i, m)]


def build_adj(m, slots, mask):
    A = [[0] * m for _ in range(m)]
    for k, (i, j) in enumerate(slots):
        if (mask >> k) & 1:
            A[i][j] = 1
            A[j][i] = 1
    return A


def is_connected(A, m):
    seen = [False] * m
    seen[0] = True
    stack = [0]
    n_seen = 1
    while stack:
        v = stack.pop()
        row = A[v]
        for u in range(m):
            if row[u] and not seen[u]:
                seen[u] = True
                n_seen += 1
                stack.append(u)
    return n_seen == m


def has_no_isolated(A, m):
    return all(any(A[i]) for i in range(m))


def check_leontovich_exact(A, m, max_n=60, max_d=12):
    """Exact-integer check; returns (True, n, d) on first hit found, else (False, None, None)."""
    w = [[1] * m]
    for _ in range(1, max_n):
        prev = w[-1]
        w.append([sum(A[i][j] * prev[j] for j in range(m)) for i in range(m)])

    homP = [0] * (max_n + 1)
    for n in range(1, max_n + 1):
        homP[n] = sum(w[n - 1])

    for d in range(2, min(max_d, max_n - 2) + 1):
        b = [w[1][i] * w[d][i] for i in range(m)]
        for stem in range(max_n - d - 1):
            n = stem + d + 2
            if n > max_n:
                break
            homE = sum(w[stem][i] * b[i] for i in range(m))
            if homE < homP[n]:
                return True, n, d
    return False, None, None


def main():
    ap = argparse.ArgumentParser(description="Exact brute-force Leontovich search")
    ap.add_argument("-m", "--vertices", type=int, required=True)
    ap.add_argument("--max-n", type=int, default=60)
    ap.add_argument("--max-d", type=int, default=12)
    ap.add_argument("--shards", type=int, default=1)
    ap.add_argument("--shard", type=int, default=0)
    args = ap.parse_args()

    m = args.vertices
    slots = slots_for(m)
    n_slots = len(slots)
    total = 1 << n_slots

    lo = total * args.shard // args.shards
    hi = total * (args.shard + 1) // args.shards

    print(
        f"m={m}: {n_slots} slots, {total:,} total graphs, "
        f"shard {args.shard}/{args.shards} -> range [{lo:,}, {hi:,})",
        file=sys.stderr,
    )

    start = time.time()
    checked = 0
    hits = 0

    for mask in range(lo, hi):
        checked += 1
        if checked % 2_000_000 == 0:
            elapsed = time.time() - start
            rate = checked / elapsed
            print(f"  ... {checked:,}/{hi - lo:,} ({rate:,.0f}/s)", file=sys.stderr)

        A = build_adj(m, slots, mask)
        if not has_no_isolated(A, m):
            continue
        if not is_connected(A, m):
            continue

        is_leo, n, d = check_leontovich_exact(A, m, args.max_n, args.max_d)
        if is_leo:
            hits += 1
            edges = [[i, j] for (i, j) in slots if A[i][j] and (i == j or True)]
            edges = [[i, j] for (i, j) in slots if A[i][j]]
            print(f"HIT mask={mask} n={n} d={d} edges={edges}")

    elapsed = time.time() - start
    print(
        f"Done shard {args.shard}/{args.shards}: checked {checked:,} graphs "
        f"in {elapsed:.1f}s, {hits} hits",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
