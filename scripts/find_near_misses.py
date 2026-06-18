#!/usr/bin/env python3
"""find_near_misses.py — Discover close near-misses and anomalies.

Searches the bipartite (3, m2) family for non-Leontovich graphs where the
homomorphism count ratio hom(E_n^(2), H) / hom(P_n, H) is extremely close to 1.0.
"""

import time


def get_min_ratio(c, n=17, d=2):
    c1, c2, c3, c4, c5, c6, c7 = c
    m2 = sum(c)
    m = 3 + m2

    adj = [[] for _ in range(m)]
    idx = 3
    # c1: {0}
    for _ in range(c1):
        adj[0].append(idx)
        adj[idx].append(0)
        idx += 1
    # c2: {1}
    for _ in range(c2):
        adj[1].append(idx)
        adj[idx].append(1)
        idx += 1
    # c3: {2}
    for _ in range(c3):
        adj[2].append(idx)
        adj[idx].append(2)
        idx += 1
    # c4: {0, 1}
    for _ in range(c4):
        adj[0].append(idx)
        adj[1].append(idx)
        adj[idx].append(0)
        adj[idx].append(1)
        idx += 1
    # c5: {0, 2}
    for _ in range(c5):
        adj[0].append(idx)
        adj[2].append(idx)
        adj[idx].append(0)
        adj[idx].append(2)
        idx += 1
    # c6: {1, 2}
    for _ in range(c6):
        adj[1].append(idx)
        adj[2].append(idx)
        adj[idx].append(1)
        adj[idx].append(2)
        idx += 1
    # c7: {0, 1, 2}
    for _ in range(c7):
        adj[0].append(idx)
        adj[1].append(idx)
        adj[2].append(idx)
        adj[idx].append(0)
        adj[idx].append(1)
        adj[idx].append(2)
        idx += 1

    # Exact DP count
    w = [[1] * m]
    for k in range(n + 1):
        prev = w[-1]
        nxt = [sum(prev[v] for v in adj[u]) for u in range(m)]
        w.append(nxt)

    b = [w[1][u] * w[d][u] for u in range(m)]
    homP = sum(w[n - 1])
    stem = n - d - 2
    homE = sum(w[stem][u] * b[u] for u in range(m))

    return homE / homP


def search_near_misses(max_m2=15, n=17):
    print(
        f"\nSearching for closest near-misses in (3, m2) up to m2={max_m2} at n={n}..."
    )
    start_time = time.time()
    candidates = []

    for m2 in range(1, max_m2 + 1):
        for c1 in range(m2 + 1):
            for c2 in range(m2 - c1 + 1):
                if c1 < c2:
                    continue
                for c3 in range(m2 - c1 - c2 + 1):
                    if c2 < c3:
                        continue
                    for c4 in range(m2 - c1 - c2 - c3 + 1):
                        for c5 in range(m2 - c1 - c2 - c3 - c4 + 1):
                            for c6 in range(m2 - c1 - c2 - c3 - c4 - c5 + 1):
                                c7 = m2 - c1 - c2 - c3 - c4 - c5 - c6

                                if c1 + c4 + c5 + c7 < 1:
                                    continue
                                if c2 + c4 + c6 + c7 < 1:
                                    continue
                                if c3 + c5 + c6 + c7 < 1:
                                    continue

                                c_vec = (c1, c2, c3, c4, c5, c6, c7)
                                ratio = get_min_ratio(c_vec, n=n)

                                if ratio > 1.000001:
                                    # Non-Leontovich, record how close it is to 1.0
                                    candidates.append((ratio, m2, c_vec))

    candidates.sort()

    print(
        "\nTop 10 closest near-misses (non-Leontovich, ratio >= 1.0, closest to 1.0):"
    )
    print("=" * 70)
    for i, (ratio, m2, c_vec) in enumerate(candidates[:10]):
        diff_pct = (ratio - 1.0) * 100
        print(
            f" {i+1:2d}. Vector: {c_vec} | Vertices: 3+{m2:<2d} | Ratio: {ratio:.8f} (+{diff_pct:.6f}%)"
        )
    print("=" * 70)
    print(f"Search completed in {time.time() - start_time:.3f} seconds.")


if __name__ == "__main__":
    search_near_misses(15, n=17)
