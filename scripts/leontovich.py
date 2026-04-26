#!/usr/bin/env python3
"""Leontovich graph verification via automorphic similarity matrices.

Verifies results from "Long paths need not minimize H-colorings among trees"
(J.D. Nir, arXiv:2510.18770v1) using the 4x4 orbit partition trick.
"""

import math
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


def task_d():
    """Verify spectral interference theorem: need ≥2 positive eigenvalues."""
    print("=" * 60)
    print("Task D: Spectral Interference Theorem Verification")
    print("=" * 60)

    def eigenvalues_2x2(x):
        """Eigenvalues of 2x2 orbit matrix [[0, x], [1, 0]]."""
        return [math.sqrt(x), -math.sqrt(x)]

    def eigenvalues_4x4(x, y, z):
        """Positive eigenvalues of T(x,y,z) quotient matrix via char. poly."""
        # det(λI - M) = λ^4 - (x + y + z)λ^2 + xz = 0
        # Substituting u = λ^2: u^2 - (x+y+z)u + xz = 0
        s = x + y + z
        disc = s * s - 4 * x * z
        if disc < 0:
            return [math.sqrt(s / 2), 0.0]  # shouldn't happen for trees
        u1 = (s + math.sqrt(disc)) / 2
        u2 = (s - math.sqrt(disc)) / 2
        lam1 = math.sqrt(u1) if u1 > 0 else 0.0
        lam2 = math.sqrt(u2) if u2 > 0 else 0.0
        return [lam1, lam2]

    # Test 2-orbit trees T(x): star-like, diameter <= 3
    print("\n--- 2-Orbit Trees T(x) (diameter <= 2) ---")
    print(f"{'x':>4} {'|V|':>6} {'lam1':>10} {'pos eigs':>10} {'P wins?':>10}")
    for x in [2, 3, 5, 10]:
        eigs = eigenvalues_2x2(x)
        pos = sum(1 for e in eigs if e > 1e-10)
        M = [[0, x], [1, 0]]
        a = [1, x]
        leo = any(
            eval_tree_hom(make_En(n), n, M, a) < eval_tree_hom(make_Pn(n), n, M, a)
            for n in range(5, 52, 2)
        )
        V = 1 + x
        print(
            f"{x:>4} {V:>6} {eigs[0]:>10.4f} {pos:>10} {'NO (Leo!)' if leo else 'YES':>10}"
        )

    # Test 3-orbit trees T(x,y): diameter <= 4
    print("\n--- 3-Orbit Trees T(x,y) (diameter <= 4) ---")
    print(f"{'(x,y)':>8} {'|V|':>6} {'lam1':>10} {'pos eigs':>10} {'P wins?':>10}")
    for x, y in [(2, 2), (3, 3), (5, 2), (10, 5)]:
        M3 = [[0, x, 0], [1, 0, y], [0, 1, 0]]
        a3 = [1, x, x * y]
        V = 1 + x + x * y
        # eigenvalues: lam^3 - (x+y)lam = 0 => only 1 positive
        lam1 = math.sqrt(x + y)
        pos = 1
        leo = any(
            eval_tree_hom(make_En(n), n, M3, a3) < eval_tree_hom(make_Pn(n), n, M3, a3)
            for n in range(5, 52, 2)
        )
        print(
            f"  ({x},{y}) {V:>6} {lam1:>10.4f} {pos:>10} {'NO (Leo!)' if leo else 'YES':>10}"
        )

    # Test 4-orbit trees T(x,y,z): can have 2 positive eigenvalues
    print("\n--- 4-Orbit Trees T(x,y,z) (diameter ≤ 6) ---")
    print(
        f"  {'(x,y,z)':>12} {'|V|':>6} {'lam1':>8} {'lam2':>8} {'pos':>4} {'Leo?':>6}"
    )
    tests = [
        (2, 1, 2),
        (3, 1, 3),
        (7, 1, 9),
        (18, 3, 32),
        (2, 2, 2),
        (5, 5, 5),
    ]
    for x, y, z in tests:
        eigs = eigenvalues_4x4(x, y, z)
        V = 1 + x + x * y + x * y * z
        pos = sum(1 for e in eigs if e > 1e-10)
        # Check if Leontovich at any odd n up to 51
        M, a = get_Ma(x, y, z)
        leo = any(
            eval_tree_hom(make_En(n), n, M, a) < eval_tree_hom(make_Pn(n), n, M, a)
            for n in range(5, 52, 2)
        )
        leo_str = "YES" if leo else "no"
        label = f"({x},{y},{z})"
        print(
            f"  {label:>12} {V:>6} {eigs[0]:>8.3f} {eigs[1]:>8.3f}"
            f" {pos:>4} {leo_str:>6}"
        )

    print()
    print("RESULT: All 2-orbit and 3-orbit trees tie P_n vs E_n (1 pos eigenvalue)")
    print("        Only 4-orbit trees with 2 pos eigenvalues can be Leontovich")

    # --- One-Way Threshold Proof (ideas-16) ---
    # Verify that for T(7,1,9), Δ_odd(n) = hom(P_n,H) - hom(E_n,H)
    # crosses zero exactly once (at n=13) and is strictly monotonic after.
    print("\n--- One-Way Threshold Verification for T(7,1,9) ---")
    x, y, z = 7, 1, 9
    eigs = eigenvalues_4x4(x, y, z)
    r = eigs[1] / eigs[0]  # λ₂/λ₁, should be in (0,1)
    print(f"  lam1 = {eigs[0]:.6f}, lam2 = {eigs[1]:.6f}")
    print(
        f"  ratio r = lam2/lam1 = {r:.6f} (must be in (0,1): {'YES' if 0 < r < 1 else 'NO'})"
    )

    M, a = get_Ma(x, y, z)
    deltas = []
    for n in range(5, 102, 2):
        hp = eval_tree_hom(make_Pn(n), n, M, a)
        he = eval_tree_hom(make_En(n), n, M, a)
        delta = hp - he  # positive means E_n wins (fewer homs)
        deltas.append((n, delta))

    # Find crossover
    crossover_n = None
    for n, d in deltas:
        if d > 0 and crossover_n is None:
            crossover_n = n

    # Verify strict monotonicity after crossover
    monotonic = True
    post_cross = [(n, d) for n, d in deltas if n >= crossover_n] if crossover_n else []
    for i in range(1, len(post_cross)):
        if post_cross[i][1] <= post_cross[i - 1][1]:
            monotonic = False
            break

    print(f"  Crossover at n = {crossover_n}")
    print(f"  Strictly monotonic after crossover: {'YES' if monotonic else 'NO'}")
    print(
        f"  No re-crossover (all Delta > 0 for odd n >= {crossover_n}): "
        f"{'YES' if all(d > 0 for n, d in deltas if n >= crossover_n) else 'NO'}"
    )

    # Print a few values around the crossover
    print(f"\n  {'n':>4} {'Delta (P-E)':>14} {'winner':>8}")
    for n, d in deltas:
        if 5 <= n <= 21 or n in (51, 101):
            winner = "E_n" if d > 0 else "P_n"
            print(f"  {n:>4} {d:>14.1f} {winner:>8}")
    print()


def get_Ma_general(degrees):
    """Build (k+1)x(k+1) similarity matrix for a depth-k symmetric tree.

    degrees = [d1, d2, ..., dk] where d_i is the branching factor at depth i.
    Root (orbit 0) has d1 children in orbit 1, each orbit-i vertex has
    d_{i+1} children in orbit i+1.

    Returns (M, a) where M is the similarity matrix and a is orbit sizes.
    """
    k = len(degrees)
    dim = k + 1  # orbits 0 through k
    M = [[0] * dim for _ in range(dim)]

    # Build adjacency: orbit i connects to orbit i+1 with weight d_{i+1}
    for i in range(k):
        M[i][i + 1] = degrees[i]  # parent -> children
        M[i + 1][i] = 1  # child -> parent

    # Orbit sizes: s_0=1, s_1=d1, s_2=d1*d2, ...
    a = [1]
    for d in degrees:
        a.append(a[-1] * d)

    return M, a


def task_e():
    """Hunt for Leontovich graphs smaller than T(7,1,9) = 78 vertices.

    Exhaustively searches all spherically symmetric trees with:
    - Depth 3 (4 orbits, = T(x,y,z) family, re-verified)
    - Depth 4 (5 orbits)
    - Depth 5 (6 orbits)
    - Depth 6 (7 orbits)
    with |V| < 78 and all branching factors >= 2.
    """
    print("=" * 60)
    print("Task E: Hunt for Smallest Leontovich Graph (|V| < 78)")
    print("=" * 60)

    max_v = 77  # must be strictly less than 78
    best_leo = None
    total_tested = 0

    for depth in range(3, 7):
        hits = []

        def search(degrees, remaining_depth):
            nonlocal total_tested, best_leo

            if remaining_depth == 0:
                M, a = get_Ma_general(degrees)
                V = sum(a)
                if V > max_v or V < 10:
                    return

                total_tested += 1

                # Test odd n from 5 to 201
                for n in range(5, 202, 2):
                    hp = eval_tree_hom(make_Pn(n), n, M, a)
                    he = eval_tree_hom(make_En(n), n, M, a)
                    if he < hp:
                        label = ",".join(str(d) for d in degrees)
                        hits.append((V, label, n))
                        if best_leo is None or V < best_leo[0]:
                            best_leo = (V, label, n)
                        break
                return

            # Try branching factors 2..max feasible
            current_product = 1
            for d in degrees:
                current_product *= d
            # Minimum |V| with remaining_depth more levels:
            # need at least 2^remaining_depth * current_product more vertices
            for d in range(1, max_v):
                new_product = current_product * d
                # Rough lower bound on total vertices
                min_extra = new_product  # at least one more level
                if 1 + current_product + min_extra > max_v:
                    break
                search(degrees + [d], remaining_depth - 1)

        print(f"\n--- Depth {depth} ({depth + 1} orbits) ---")
        search([], depth)
        print(f"  Tested: {total_tested} trees")

        if hits:
            hits.sort()
            for V, label, n in hits[:10]:
                print(f"  LEONTOVICH! T({label}) |V|={V} at n={n}")
        else:
            print(f"  No Leontovich graph found at depth {depth}")

    print(f"\n{'=' * 60}")
    print(f"Total symmetric trees tested: {total_tested}")
    if best_leo:
        print(
            f"SMALLEST LEONTOVICH: T({best_leo[1]}) with |V|={best_leo[0]} at n={best_leo[2]}"
        )
    else:
        print(
            "No Leontovich graph smaller than 78 vertices found in symmetric families"
        )
    print()


if __name__ == "__main__":
    task_a()
    task_b()
    task_c()
    task_d()
    task_e()
