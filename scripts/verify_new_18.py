#!/usr/bin/env python3
"""verify_new_18.py — Reconstruct and verify the newly discovered 18-vertex Leontovich graph.

Partition sizes: m1 = 3, m2 = 15.
Triplet counts: c1=7, c2=0, c3=0, c4=1, c5=1, c6=6, c7=0.
"""

from leontovich_smt import to_graph6
from verify_hom import verify_graph


def main():
    m = 18
    adj = [[0] * m for _ in range(m)]

    # Left partition: 0, 1, 2
    # Right partition: 3..17
    # c1 = 7 vertices connected to {0} (3, 4, 5, 6, 7, 8, 9)
    for j in range(3, 10):
        adj[0][j] = adj[j][0] = 1
    # c2 = 0
    # c3 = 0
    # c4 = 1 vertex connected to {0, 1} (10)
    adj[0][10] = adj[10][0] = 1
    adj[1][10] = adj[10][1] = 1
    # c5 = 1 vertex connected to {0, 2} (11)
    adj[0][11] = adj[11][0] = 1
    adj[2][11] = adj[11][2] = 1
    # c6 = 6 vertices connected to {1, 2} (12, 13, 14, 15, 16, 17)
    for j in range(12, 18):
        adj[1][j] = adj[j][1] = 1
        adj[2][j] = adj[j][2] = 1
    # c7 = 0

    g6 = to_graph6(adj)
    print(f"Generated Graph6: {g6}")

    # Verify using the official exact verifier
    verify_graph(g6)


if __name__ == "__main__":
    main()
