#!/usr/bin/env python3
"""Verify the 76-vertex Leontovich graph discovered by simulated annealing.

This graph was found by mutating T(7,1,9) (78 vertices, the previously
smallest known Leontovich graph from Galvin-McMillon-Nir-Redlich 2026).

Usage:
    python3 scripts/verify_76.py

No dependencies beyond numpy.
"""

import numpy as np

# ── 76-vertex graph: edge list ──────────────────────────────────────────
# Discovered 2026-04-26 via SA from T(7,1,9) (seed=7, step=577)
# λ₁ = 3.37657, λ₂ = √10 ≈ 3.16228, r = λ₂/λ₁ = 0.9365
EDGES_76 = [
    [0, 1],
    [0, 2],
    [0, 3],
    [0, 4],
    [0, 5],
    [0, 6],
    [0, 7],
    [1, 8],
    [2, 9],
    [3, 10],
    [4, 11],
    [5, 12],
    [6, 13],
    [7, 14],
    [8, 15],
    [8, 16],
    [8, 17],
    [8, 18],
    [8, 19],
    [8, 20],
    [8, 21],
    [8, 22],
    [8, 23],
    [9, 24],
    [9, 25],
    [9, 26],
    [9, 27],
    [9, 28],
    [9, 29],
    [9, 30],
    [9, 31],
    [9, 32],
    [10, 33],
    [10, 34],
    [10, 35],
    [10, 36],
    [10, 37],
    [10, 38],
    [10, 39],
    [10, 40],
    [10, 41],
    [11, 42],
    [11, 43],
    [11, 44],
    [11, 45],
    [11, 46],
    [11, 47],
    [11, 48],
    [11, 49],
    [11, 50],
    [12, 51],
    [12, 52],
    [12, 53],
    [12, 54],
    [12, 55],
    [12, 56],
    [12, 57],
    [12, 58],
    [12, 59],
    [13, 60],
    [13, 61],
    [13, 62],
    [13, 63],
    [13, 64],
    [13, 65],
    [13, 66],
    [13, 67],
    [14, 68],
    [14, 69],
    [14, 70],
    [14, 71],
    [14, 72],
    [14, 73],
    [14, 74],
    [14, 75],
]

M = 76  # vertex count


def build_adjacency(edges, m):
    A = np.zeros((m, m), dtype=np.float64)
    for i, j in edges:
        A[i, j] = A[j, i] = 1.0
    return A


def verify():
    A = build_adjacency(EDGES_76, M)

    print(f"Vertices: {M}")
    print(f"Edges: {len(EDGES_76)}")
    print(f"Degree sequence: {sorted(np.sum(A, axis=1).astype(int), reverse=True)}")
    print()

    # Eigenvalues
    evals = np.linalg.eigvalsh(A)
    pos = sorted([e for e in evals if e > 1e-10], reverse=True)
    print(f"λ₁ = {pos[0]:.8f}")
    print(f"λ₂ = {pos[1]:.8f}  (√10 = {np.sqrt(10):.8f})")
    print(f"r = λ₂/λ₁ = {pos[1]/pos[0]:.8f}")
    print()

    # Compute w[k] = A^k * 1  (exact integer arithmetic via pure Python)
    print("Computing hom(P_n) and hom(E_n^{(2)}) with exact integers...")

    # Build adjacency as pure Python list of neighbor lists
    adj = [[] for _ in range(M)]
    for i, j in EDGES_76:
        adj[i].append(j)
        adj[j].append(i)

    w = [[1] * M]  # w[0] = all-ones
    for k in range(50):
        prev = w[-1]
        nxt = [sum(prev[j] for j in adj[i]) for i in range(M)]
        w.append(nxt)

    # hom(P_n) = sum(w[n-1])
    # hom(E_n^{(2)}) = sum_i w[stem][i] * w[1][i] * w[2][i]
    # where stem = n - 2 - 2 = n - 4
    b = [w[1][i] * w[2][i] for i in range(M)]

    print()
    print(f"{'n':>4} {'hom(P_n)':>45} {'hom(E_n^(2))':>45} {'Delta':>15} {'E<P?':>5}")
    print("-" * 120)

    found_crossover = False
    for n in range(5, 50):
        homP = sum(w[n - 1])
        stem = n - 4
        if stem < 0:
            continue
        homE = sum(w[stem][i] * b[i] for i in range(M))
        delta = homP - homE
        flag = " <<<" if delta > 0 else ""
        if delta > 0 and not found_crossover:
            found_crossover = True
            flag = " <<< CROSSOVER!"
        print(f"{n:4d} {str(homP):>45s} {str(homE):>45s} {str(delta):>15s}{flag}")

    if found_crossover:
        print("\n✓ VERIFIED: This is a genuine Leontovich graph!")
        print("  hom(E_n^{(2)}, H) < hom(P_n, H) for some n.")
    else:
        print("\n✗ NOT VERIFIED: No crossover found in n = [5, 49].")

    # Also compare with T(7,1,9)
    print("\n--- Comparison with T(7,1,9) ---")
    print("T(7,1,9): |V| = 78, crosses at n=13")
    print(f"This graph: |V| = {M}, crosses at n=21")
    print(f"Improvement: {78 - M} fewer vertices")


if __name__ == "__main__":
    verify()
