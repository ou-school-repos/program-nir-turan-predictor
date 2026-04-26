#!/usr/bin/env python3
"""Leontovich graph verification via automorphic similarity matrices.

Verifies results from "Long paths need not minimize H-colorings among trees"
(J.D. Nir, arXiv:2510.18770v1) using the 4x4 orbit partition trick.
"""

from collections import deque


def get_Ma(x, y, z):
    """Build 4x4 automorphic similarity matrix and orbit sizes for T(x,y,z)."""
    M = [
        [0, x, 0, 0],
        [1, 0, y, 0],
        [0, 1, 0, z],
        [0, 0, 1, 0],
    ]
    a = [1, x, x * y, x * y * z]
    return M, a


def eval_tree_hom(edges, n, M, a):
    """Compute hom(T, H) where H has similarity matrix M and orbit sizes a."""
    k = len(a)
    adj = [[] for _ in range(n)]
    for u, v in edges:
        adj[u].append(v)
        adj[v].append(u)

    # BFS to get parent array
    order = []
    parent = [-1] * n
    visited = [False] * n
    visited[0] = True
    q = deque([0])
    while q:
        u = q.popleft()
        order.append(u)
        for v in adj[u]:
            if not visited[v]:
                visited[v] = True
                parent[v] = u
                q.append(v)

    # Bottom-up DP
    dp = [[1] * k for _ in range(n)]
    for u in reversed(order):
        p = parent[u]
        if p != -1:
            for cp in range(k):
                dp[p][cp] *= sum(M[cp][d] * dp[u][d] for d in range(k))

    return sum(a[i] * dp[0][i] for i in range(k))


def make_Pn(n):
    """Path graph P_n."""
    return [(i, i + 1) for i in range(n - 1)]


def make_En(n):
    """E_n: P_{n-1} with pendant edge from vertex third-from-end."""
    edges = make_Pn(n - 1)
    if n >= 4:
        edges.append((n - 4, n - 1))
    return edges


def task_a():
    """Verify Theorem 1.3: T(18,3,32) is Leontovich at n=7."""
    print("=" * 60)
    print("Task A: Verify Theorem 1.3 — T(18,3,32) at n=7")
    print("=" * 60)
    x, y, z = 18, 3, 32
    M, a = get_Ma(x, y, z)
    V = sum(a)
    hP7 = eval_tree_hom(make_Pn(7), 7, M, a)
    hE7 = eval_tree_hom(make_En(7), 7, M, a)
    print(f"H = T({x}, {y}, {z}), |V(H)| = {V}")
    print(f"hom(P_7, H) = {hP7:,}")
    print(f"hom(E_7, H) = {hE7:,}")
    print(f"Difference:    {hP7 - hE7:,}")
    if hE7 < hP7:
        print("VERIFIED: hom(E_7, H) < hom(P_7, H)")
    else:
        print("FAILED!")
    print()


def task_b():
    """Find exact threshold n where T(7,1,9) becomes Leontovich."""
    print("=" * 60)
    print("Task B: Exact threshold for T(7,1,9)")
    print("=" * 60)
    x, y, z = 7, 1, 9
    M, a = get_Ma(x, y, z)
    V = sum(a)
    print(f"H = T({x}, {y}, {z}), |V(H)| = {V}")
    print()

    print(
        f"{'n':>4} {'parity':>6} {'hom(P_n)':>20} {'hom(E_n)':>20} {'diff':>15} {'winner':>8}"
    )
    print("-" * 80)

    found_odd = None
    for n in range(5, 52):
        hp = eval_tree_hom(make_Pn(n), n, M, a)
        he = eval_tree_hom(make_En(n), n, M, a)
        diff = hp - he
        parity = "odd" if n % 2 == 1 else "even"
        winner = "E_n" if he < hp else "P_n" if hp < he else "TIE"
        print(f"{n:>4} {parity:>6} {hp:>20,} {he:>20,} {diff:>15,} {winner:>8}")

        if he < hp and n % 2 == 1 and found_odd is None:
            found_odd = n

    print()
    if found_odd:
        print(f"RESULT: Smallest odd n where E_n beats P_n: n = {found_odd}")
    else:
        print("No odd threshold found in range")
    print()


def task_c():
    """Search for smallest T(x,y,z) that is Leontovich."""
    print("=" * 60)
    print("Task C: Smallest Leontovich T(x,y,z) with |V| <= 100")
    print("=" * 60)

    best_V = 101
    best_params = None
    best_n = None

    for x in range(1, 100):
        for y in range(1, 100):
            if 1 + x + x * y > 100:
                break
            for z in range(1, 100):
                V = 1 + x + x * y + x * y * z
                if V >= best_V:
                    break
                M, a = get_Ma(x, y, z)
                for n in range(5, 50):
                    he = eval_tree_hom(make_En(n), n, M, a)
                    hp = eval_tree_hom(make_Pn(n), n, M, a)
                    if he < hp:
                        best_V = V
                        best_params = (x, y, z)
                        best_n = n
                        print(
                            f"  New best: T({x},{y},{z}), |V|={V}, "
                            f"Leontovich at n={n}"
                        )
                        break

    print()
    if best_params:
        x, y, z = best_params
        print(f"RESULT: Smallest T(x,y,z) = T({x},{y},{z})")
        print(f"        |V| = {best_V}, first Leontovich at n = {best_n}")
    else:
        print("No Leontovich T(x,y,z) found with |V| < 100")
    print()


if __name__ == "__main__":
    task_a()
    task_b()
    task_c()
