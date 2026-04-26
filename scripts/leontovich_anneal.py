#!/usr/bin/env python3
"""Simulated annealing search for small Leontovich graphs.

Starts from the known T(7,1,9) Leontovich graph (78 vertices) and
explores graph space by edge mutations, seeking smaller graphs that
still exhibit a Leontovich crossover (hom(E_n) < hom(P_n) for some n).

Usage:
    python3 scripts/leontovich_anneal.py [--steps 100000] [--temp 1.0]
"""

import argparse
import json
import math
import random
import sys
from datetime import datetime

import numpy as np


def make_T(x, y, z):
    """Build adjacency matrix for T(x,y,z) symmetric tree."""
    # Orbits: root (1), children of root (x), bridge (xy), leaves (xyz)
    n = 1 + x + x * y + x * y * z
    A = np.zeros((n, n), dtype=np.float64)

    root = 0
    children = list(range(1, 1 + x))
    bridges = list(range(1 + x, 1 + x + x * y))
    leaves = list(range(1 + x + x * y, n))

    # root -> children
    for c in children:
        A[root, c] = A[c, root] = 1

    # children -> bridges (y bridges per child)
    for i, c in enumerate(children):
        for j in range(y):
            b = bridges[i * y + j]
            A[c, b] = A[b, c] = 1

    # bridges -> leaves (z leaves per bridge)
    for i, b in enumerate(bridges):
        for j in range(z):
            lf = leaves[i * z + j]
            A[b, lf] = A[lf, b] = 1

    return A


def check_leontovich(A, max_n=200, max_d=20):
    """Check if graph with adjacency A is Leontovich.

    Returns (is_leontovich, best_n, best_d, ratio) or (False, 0, 0, inf).
    """
    m = A.shape[0]
    ones = np.ones(m)

    # Precompute w[k] = A^k * 1
    w = [ones.copy()]
    for k in range(max_n):
        w.append(A @ w[-1])

    # hom(P_n) = sum(w[n-1])
    homP = [0.0] + [np.sum(w[k]) for k in range(max_n)]

    best_ratio = float("inf")
    best_n = 0
    best_d = 0

    for d in range(2, min(max_d + 1, max_n)):
        b = w[1] * w[d]  # branch factor
        for stem in range(max_n - d - 1):
            n = stem + d + 2
            if n > max_n:
                break
            homE = np.dot(w[stem], b)
            if homP[n] > 0:
                ratio = homE / homP[n]
                if ratio < best_ratio:
                    best_ratio = ratio
                    best_n = n
                    best_d = d

    return best_ratio < 1.0 - 1e-11, best_n, best_d, best_ratio


def spectral_ratio(A):
    """Compute r = lambda_2 / lambda_1 for graph adjacency A."""
    evals = np.linalg.eigvalsh(A)
    pos = sorted([e for e in evals if e > 1e-10], reverse=True)
    if len(pos) < 2:
        return 0.0, pos[0] if pos else 0.0, 0.0
    return pos[1] / pos[0], pos[0], pos[1]


def random_bipartite_graph(n1, n2):
    """Generate a random connected bipartite graph."""
    m = n1 + n2
    A = np.zeros((m, m), dtype=np.float64)
    # Start with a spanning tree (path alternating between halves)
    left = list(range(n1))
    right = list(range(n1, m))
    random.shuffle(left)
    random.shuffle(right)

    # Connect in a zig-zag to ensure connectivity
    for i in range(max(n1, n2)):
        li = left[i % n1]
        ri = right[i % n2]
        A[li, ri] = A[ri, li] = 1

    # Add some random edges
    extra = random.randint(0, n1 * n2 // 4)
    for _ in range(extra):
        li = random.choice(left)
        ri = random.choice(right)
        A[li, ri] = A[ri, li] = 1

    return A


def mutate_graph(A, n1):
    """Mutate a bipartite graph by flipping one edge."""
    m = A.shape[0]
    A_new = A.copy()

    action = random.random()
    if action < 0.5:
        # Flip a random bipartite edge
        i = random.randint(0, n1 - 1)
        j = random.randint(n1, m - 1)
        A_new[i, j] = 1.0 - A_new[i, j]
        A_new[j, i] = A_new[i, j]
    else:
        # Swap two edges (rewire)
        i1 = random.randint(0, n1 - 1)
        j1 = random.randint(n1, m - 1)
        i2 = random.randint(0, n1 - 1)
        j2 = random.randint(n1, m - 1)
        A_new[i1, j1], A_new[i2, j2] = A_new[i2, j2], A_new[i1, j1]
        A_new[j1, i1], A_new[j2, i2] = A_new[j2, i2], A_new[j1, i1]

    # Check connectivity via BFS
    visited = set()
    queue = [0]
    visited.add(0)
    while queue:
        v = queue.pop()
        for u in range(m):
            if A_new[v, u] > 0 and u not in visited:
                visited.add(u)
                queue.append(u)

    if len(visited) < m:
        return A, False  # disconnected, reject

    # Check still has edges
    if np.sum(A_new) == 0:
        return A, False

    return A_new, True


def shrink_graph(A, n1):
    """Try to remove a vertex while maintaining connectivity and bipartiteness."""
    m = A.shape[0]
    n2 = m - n1

    # Pick a random vertex to remove
    if random.random() < 0.5 and n1 > 2:
        # Remove from left partition
        v = random.randint(0, n1 - 1)
        new_n1 = n1 - 1
    elif n2 > 2:
        # Remove from right partition
        v = random.randint(n1, m - 1)
        new_n1 = n1
    else:
        return A, n1, False

    # Delete vertex v
    A_new = np.delete(np.delete(A, v, axis=0), v, axis=1)

    # Check connectivity
    m_new = A_new.shape[0]
    visited = set()
    queue = [0]
    visited.add(0)
    while queue:
        node = queue.pop()
        for u in range(m_new):
            if A_new[node, u] > 0 and u not in visited:
                visited.add(u)
                queue.append(u)

    if len(visited) < m_new:
        return A, n1, False

    return A_new, new_n1, True


def anneal(steps=100000, temp_init=1.0, seed_graph="T(7,1,9)"):
    """Simulated annealing to find small Leontovich graphs."""
    print("=== Leontovich Simulated Annealing ===", file=sys.stderr)
    print(f"Steps: {steps}, Temp: {temp_init}", file=sys.stderr)

    # Initialize from T(7,1,9)
    A = make_T(7, 1, 9)
    n1 = 1 + 7 + 7  # root + children + bridges = 15 left-side vertices
    m = A.shape[0]

    is_leo, best_n, best_d, ratio = check_leontovich(A, max_n=100)
    r, lam1, lam2 = spectral_ratio(A)

    print(
        f"Seed: {seed_graph}, |V|={m}, leontovich={is_leo}, "
        f"ratio={ratio:.6f}, r={r:.4f}",
        file=sys.stderr,
    )

    # Objective: minimize |V| while staying Leontovich
    # Non-Leontovich states get a steep penalty to prevent collapse
    def score(A_cand, is_leo_cand, ratio_cand):
        sz = A_cand.shape[0]
        if is_leo_cand:
            return float(sz)  # pure vertex count
        else:
            # Heavy penalty: "distance from crossover" keeps graph near boundary
            return 500.0 + sz + 100.0 * max(0, ratio_cand - 0.999)

    current_score = score(A, is_leo, ratio)
    best_m = m
    best_is_leo = is_leo

    hits = []

    for step in range(steps):
        temp = temp_init * (1.0 - step / steps)

        action = random.random()
        if action < 0.10 and m > 16:
            # Shrink: only 10% of the time, and only if still large
            A_cand, n1_cand, ok = shrink_graph(A, n1)
            if not ok:
                continue
        else:
            # Edge mutation: 90% of the time
            A_cand, ok = mutate_graph(A, n1)
            n1_cand = n1
            if not ok:
                continue

        is_leo_cand, n_cand, d_cand, ratio_cand = check_leontovich(A_cand, max_n=100)
        cand_score = score(A_cand, is_leo_cand, ratio_cand)

        # Metropolis acceptance
        delta = cand_score - current_score
        if delta < 0 or (
            temp > 0 and random.random() < math.exp(-delta / max(temp, 0.01))
        ):
            A = A_cand
            n1 = n1_cand
            m = A.shape[0]
            current_score = cand_score
            is_leo = is_leo_cand
            ratio = ratio_cand

            if is_leo_cand and m < best_m:
                best_m = m
                best_is_leo = True
                r, lam1, lam2 = spectral_ratio(A)

                hit = {
                    "step": step,
                    "m": m,
                    "n": n_cand,
                    "d": d_cand,
                    "ratio": round(ratio_cand, 8),
                    "r": round(r, 6),
                    "lam1": round(lam1, 6),
                    "lam2": round(lam2, 6),
                    "time": datetime.now().isoformat(),
                }
                hits.append(hit)
                print(json.dumps(hit))
                print(
                    f"  [SA] step={step} NEW BEST |V|={m} n={n_cand} "
                    f"d={d_cand} ratio={ratio_cand:.6f} r={r:.4f}",
                    file=sys.stderr,
                )

        if step % 5000 == 0:
            print(
                f"  [SA] step={step}/{steps} |V|={m} "
                f"leo={is_leo} ratio={ratio:.6f} best={best_m} "
                f"T={temp:.4f}",
                file=sys.stderr,
            )

    print("\n=== Final Results ===", file=sys.stderr)
    print(
        f"Best Leontovich graph: |V|={best_m}, " f"found={best_is_leo}",
        file=sys.stderr,
    )
    print(f"Total hits below seed size: {len(hits)}", file=sys.stderr)

    return hits


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SA search for Leontovich graphs")
    parser.add_argument("--steps", type=int, default=100000)
    parser.add_argument("--temp", type=float, default=2.0)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    random.seed(args.seed)
    np.random.seed(args.seed)

    anneal(steps=args.steps, temp_init=args.temp)
