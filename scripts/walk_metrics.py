#!/usr/bin/env python3
"""walk_metrics.py — Compute advanced walk metrics for the 18-vertex bipartite Leontovich target.

Group names are alphabetically sorted and highly intuitive:
  - L0, L1, L2: Left bipartition nodes
  - R_01: Right-hand node connected to {v0, v1}
  - R_02: Right-hand node connected to {v0, v2}
  - R_12: Right-hand core nodes connected to {v1, v2}
  - R_leaf_0: Right-hand leaf nodes connected only to v0
"""

import math


def main():
    m = 18
    # Adjacency list representation
    adj = {i: [] for i in range(m)}

    # Pattern vector: (7, 0, 0, 1, 1, 6, 0)
    # R_leaf_0: 7 leaves connected to L0
    for r in range(3, 10):
        adj[0].append(r)
        adj[r].append(0)

    # R_01: 1 node connected to L0 and L1
    for l in [0, 1]:
        adj[l].append(10)
        adj[10].append(l)

    # R_02: 1 node connected to L0 and L2
    for l in [0, 2]:
        adj[l].append(11)
        adj[11].append(l)

    # R_12: 6 nodes connected to L1 and L2
    for r in range(12, 18):
        for l in [1, 2]:
            adj[l].append(r)
            adj[r].append(l)

    # w[k][u]: walks of length k starting at u
    w = [[1] * m]
    max_steps = 22
    for step in range(1, max_steps + 1):
        prev = w[-1]
        nxt = [sum(prev[v] for v in adj[u]) for u in range(m)]
        w.append(nxt)

    print("=" * 105)
    print(
        f"{'n':<4} | {'hom(P_n, H)':<15} | {'hom(E_n^(2), H)':<15} | {'Ratio':<8} | {'Growth Factor':<12} | {'Left Walk %':<12} | {'Right Walk %':<12}"
    )
    print("=" * 105)

    for n in range(5, max_steps + 1, 2):
        homP = sum(w[n - 1])
        # d = 2
        stem = n - 4
        homE = sum(w[stem][u] * w[1][u] * w[2][u] for u in range(m))
        ratio = homE / homP

        # Growth factor converges to the spectral radius (principal eigenvalue lambda_1)
        if n >= 7:
            prev_homP = sum(w[n - 3])
            growth = math.sqrt(homP / prev_homP)
            growth_str = f"{growth:.6f}"
        else:
            growth_str = "N/A"

        # Bipartition walk concentrations (demonstrating bipartite parity oscillations)
        left_sum = sum(w[n - 1][u] for u in range(3))
        right_sum = sum(w[n - 1][u] for u in range(3, 18))
        total_sum = left_sum + right_sum
        left_pct = (left_sum / total_sum) * 100
        right_pct = (right_sum / total_sum) * 100

        print(
            f"{n:<4} | {homP:<15d} | {homE:<15d} | {ratio:.6f} | {growth_str:<12} | {left_pct:.2f}% | {right_pct:.2f}%"
        )

    print("=" * 105)
    print("\nSTEP-BY-STEP SYMMETRY GROUPED WALK VECTORS (ALPHABETICAL ORDER):")
    print("-" * 105)
    print(
        f"{'Step k':<8} | {'L0':<7} | {'L1':<7} | {'L2':<7} | {'R_01 (x1)':<9} | {'R_02 (x1)':<9} | {'R_12 (x6)':<9} | {'R_leaf_0 (x7)':<13} | {'Total Sum':<10}"
    )
    print("-" * 105)
    for k in range(6):
        total_sum = sum(w[k])
        print(
            f"w_{k:<6} | {w[k][0]:<7d} | {w[k][1]:<7d} | {w[k][2]:<7d} | {w[k][10]:<9d} | {w[k][11]:<9d} | {w[k][12]:<9d} | {w[k][3]:<13d} | {total_sum:<10d}"
        )
    print("-" * 105)


if __name__ == "__main__":
    main()
