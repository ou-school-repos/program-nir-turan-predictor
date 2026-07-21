#!/usr/bin/env python3
"""
Independent verification of H-coloring minimizer results.

Uses networkx to enumerate all non-isomorphic trees on n vertices
and compute hom(T, P_k) via tree DP. Outputs:
  - Tree count (should match OEIS A000055)
  - min hom(T, P_k) and which tree achieves it
  - 2nd-min hom(T, P_k)
  - Aggregate checksum: sum of hom(T, P_k) over all trees
  - Star value: hom(K_{1,n-1}, P_k)

This script is intentionally simple so a reviewer can audit it
in under 5 minutes. No external dependencies beyond networkx/numpy.

Usage:
    python3 scripts/verify_hcoloring.py 10 5    # n=10, target P_5
    python3 scripts/verify_hcoloring.py 15 3    # n=15, target P_3
"""

import sys
import time

import networkx as nx
import numpy as np


def hom_count(T: nx.Graph, A: np.ndarray) -> int:
    """Count graph homomorphisms from tree T to target with adjacency A.

    Uses bottom-up DP on the rooted tree:
      dp[v][c] = product over children u of (sum_{c' ~ c} dp[u][c'])
    Total = sum_c dp[root][c]
    """
    h = A.shape[0]
    root = 0
    order = list(nx.dfs_postorder_nodes(T, root))
    parent = {root: None}
    for u, v in nx.bfs_edges(T, root):
        parent[v] = u

    dp = {v: np.ones(h, dtype=np.int64) for v in T.nodes()}
    for v in order:
        if parent[v] is not None:
            p = parent[v]
            child_contrib = A @ dp[v]  # child_contrib[c] = sum_{c'~c} dp[v][c']
            dp[p] *= child_contrib

    return int(dp[root].sum())


def star_hom(n: int, A: np.ndarray) -> int:
    """Compute hom(K_{1,n-1}, H) analytically.

    Center maps to c, each of (n-1) leaves maps to a neighbor of c.
    """
    if n <= 1:
        return A.shape[0] if n == 1 else 1
    h = A.shape[0]
    total = 0
    for c in range(h):
        nbrs = int(A[c].sum())
        total += nbrs ** (n - 1)
    return total


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} N K")
        print("  N = number of vertices in tree")
        print("  K = number of vertices in target path P_K")
        sys.exit(1)

    n = int(sys.argv[1])
    k_arg = sys.argv[2]
    k = int(k_arg.lstrip("Pp")) if k_arg[0] in "Pp" else int(k_arg)

    print(f"=== Verifying hom(T, P_{k}) for all trees on n={n} vertices ===")
    print()

    # Build target path P_k
    H = nx.path_graph(k)
    A = nx.adjacency_matrix(H).toarray().astype(np.int64)

    # Enumerate all non-isomorphic trees
    t0 = time.time()
    trees = list(nx.nonisomorphic_trees(n))
    elapsed_enum = time.time() - t0
    print(f"Trees enumerated: {len(trees)}  ({elapsed_enum:.1f}s)")

    # Compute hom(T, P_k) for each tree
    t0 = time.time()
    scores = []
    for T in trees:
        scores.append(hom_count(T, A))
    elapsed_hom = time.time() - t0

    # Results
    tree_count = len(scores)
    min_score = min(scores)
    sorted_scores = sorted(scores)
    second_min = sorted_scores[1] if len(sorted_scores) > 1 else min_score
    checksum = sum(scores)
    star = star_hom(n, A)

    # Path baseline
    P_n = nx.path_graph(n)
    path_score = hom_count(P_n, A)

    # Find minimizer topology
    min_idx = scores.index(min_score)
    min_tree = trees[min_idx]
    min_deg = max(dict(min_tree.degree()).values()) if n > 0 else 0
    is_path = min_deg <= 2

    print(f"Computed hom for {tree_count} trees  ({elapsed_hom:.1f}s)")
    print()
    print(f"--- Results (n={n}, target=P_{k}) ---")
    print(f"  trees:     {tree_count}")
    print(f"  path:      hom(P_{n}, P_{k}) = {path_score}")
    print(f"  star:      hom(K_1,{n - 1}, P_{k}) = {star}")
    print(f"  min:       {min_score}  (Delta_max={min_deg}, is_path={is_path})")
    print(f"  2nd-min:   {second_min}")
    print(f"  checksum:  {checksum}")
    print(f"  violation: {min_score < path_score}")
    print()

    if min_score == path_score:
        print(
            f"CONFIRMED: Path P_{n} minimizes hom(T, P_{k}) among all {tree_count} trees."
        )
    else:
        print(f"VIOLATION: Found tree with hom={min_score} < path={path_score}!")
        print(f"  Minimizer edges: {list(min_tree.edges())}")


if __name__ == "__main__":
    main()
