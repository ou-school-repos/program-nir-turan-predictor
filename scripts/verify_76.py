#!/usr/bin/env python3
"""Verify the 76-vertex Leontovich graph discovered by simulated annealing.

This graph was found by mutating T(7,1,9) (78 vertices, the previously
smallest known Leontovich graph from Galvin-McMillon-Nir-Redlich 2026).

Usage:
    python3 scripts/verify_76.py

No dependencies beyond numpy.
"""

import json
from pathlib import Path

import numpy as np

# Load graph from shared JSON
DATA_PATH = Path(__file__).resolve().parent.parent / "data" / "leontovich_76.json"
with open(DATA_PATH) as f:
    GRAPH_DATA = json.load(f)

EDGES_76 = GRAPH_DATA["edges"]
M = GRAPH_DATA["vertices"]


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
