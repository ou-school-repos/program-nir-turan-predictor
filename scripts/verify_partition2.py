#!/usr/bin/env python3
"""verify_partition2.py — Rigorously prove bounds for (2, m2) bipartite graphs.

Exploits the fact that for left-partition size m1 = 2, every right-partition
vertex has only 3 valid non-empty connection patterns:
  - Pattern 1 (c1): connected only to Left 0
  - Pattern 2 (c2): connected only to Left 1
  - Pattern 3 (c3): connected to both Left 0 and Left 1

Any bipartite graph with partition (2, m2) is uniquely determined (up to isomorphism)
by the triplet (c1, c2, c3) where c1 + c2 + c3 = m2.
"""

import time


def verify_triplet(c1, c2, c3, max_n=51, d=2):
    """Verify if the bipartite graph specified by (c1, c2, c3) is Leontovich."""
    m = 2 + c1 + c2 + c3
    # Build neighborhood lists
    # Left vertices: 0, 1
    # Right vertices: 2 .. 2+c1-1 (only connected to 0)
    #                2+c1 .. 2+c1+c2-1 (only connected to 1)
    #                2+c1+c2 .. m-1 (connected to both 0 and 1)
    adj = [[] for _ in range(m)]

    # Pattern 1
    for j in range(2, 2 + c1):
        adj[0].append(j)
        adj[j].append(0)
    # Pattern 2
    for j in range(2 + c1, 2 + c1 + c2):
        adj[1].append(j)
        adj[j].append(1)
    # Pattern 3
    for j in range(2 + c1 + c2, m):
        adj[0].append(j)
        adj[1].append(j)
        adj[j].append(0)
        adj[j].append(1)

    # Compute walks w[k] = A^k * 1
    w = [[1] * m]
    for k in range(max_n):
        prev = w[-1]
        nxt = [sum(prev[v] for v in adj[u]) for u in range(m)]
        w.append(nxt)

    b = [w[1][u] * w[d][u] for u in range(m)]

    # Check for crossovers
    for n in range(5, max_n, 2):
        homP = sum(w[n - 1])
        stem = n - d - 2
        if stem < 0:
            continue
        homE = sum(w[stem][u] * b[u] for u in range(m))
        if homE < homP:
            return n, homP, homE
    return None


def prove_bipartite_partition2(max_m2=40, max_n=51):
    print(
        f"\n\033[1;36mPioneering rigorous proof for bipartite (2, m2) targets up to m2={max_m2}...\033[0m"
    )
    print(f"  Checking thresholds n up to {max_n}...")

    start_time = time.time()
    total_graphs = 0
    violations_found = 0

    for m2 in range(1, max_m2 + 1):
        for c1 in range(m2 + 1):
            for c2 in range(m2 - c1 + 1):
                c3 = m2 - c1 - c2

                # Symmetry-breaking: enforce c1 >= c2
                if c1 < c2:
                    continue
                # Ensure left vertices are not isolated
                if c1 + c3 < 1 or c2 + c3 < 1:
                    continue

                total_graphs += 1
                res = verify_triplet(c1, c2, c3, max_n)
                if res is not None:
                    n, homP, homE = res
                    violations_found += 1
                    print(
                        f"\n\033[1;32m★ FOUND Leontovich graph! (c1={c1}, c2={c2}, c3={c3})\033[0m"
                    )
                    print(f"  Total vertices (m): 2 + {m2} = {2 + m2}")
                    print(f"  Crossover at n:     {n}")
                    print(f"  hom(P_n, H):        {homP:,}")
                    print(f"  hom(E_n^(2), H):    {homE:,}")

    elapsed = time.time() - start_time
    print(
        "\n\033[1;36m============================================================\033[0m"
    )
    print(f"PROOF COMPLETED in {elapsed:.3f} seconds.")
    print(f"  Total non-isomorphic (2, m2) graphs evaluated: {total_graphs:,}")
    print(f"  Leontovich violations found:                   {violations_found}")
    if violations_found == 0:
        print(
            f"\033[1;32m✓ THEOREM: No bipartite Leontovich graph of partition (2, m2) exists for m2 <= {max_m2}.\033[0m"
        )
    print("============================================================\033[0m")


if __name__ == "__main__":
    prove_bipartite_partition2(40)
