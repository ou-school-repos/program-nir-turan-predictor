#!/usr/bin/env python3
"""Verify the minimal 5-orbit strongly Leontovich tree T(1, 12, 1, 18)."""


def verify_strong_leontovich(d1, d2, d3, d4, max_n=50):
    print(f"=== Verifying 5-Orbit Looped Symmetric Tree T^({d1}, {d2}, {d3}, {d4}) ===")

    # Vertex count calculation
    V = 1 + d1 + d1 * d2 + d1 * d2 * d3 + d1 * d2 * d3 * d4
    print(f"|V| = {V}")
    print()

    # The quotient matrix Q
    Q = [
        [1, d1, 0, 0, 0],
        [1, 0, d2, 0, 0],
        [0, 1, 0, d3, 0],
        [0, 0, 1, 0, d4],
        [0, 0, 0, 1, 0],
    ]

    # Orbit sizes
    a = [1, d1, d1 * d2, d1 * d2 * d3, d1 * d2 * d3 * d4]

    # Exact integer walk vectors w[k] = Q^k * 1
    w = [[1, 1, 1, 1, 1]]
    for k in range(1, max_n + 1):
        nxt = [0, 0, 0, 0, 0]
        for i in range(5):
            nxt[i] = sum(Q[i][j] * w[-1][j] for j in range(5))
        w.append(nxt)

    def homP(n):
        return sum(a[i] * w[n - 1][i] for i in range(5))

    def homE(n):
        stem = n - 4
        if stem < 0:
            return None
        return sum(a[i] * w[stem][i] * w[1][i] * w[2][i] for i in range(5))

    print(
        f"{'n':>4} {'hom(P_n)':>25} {'hom(E_n)':>25} {'Delta (P_n - E_n)':>25} {'flag':>10}"
    )
    print("-" * 95)

    crossover_n = None
    for n in range(5, max_n + 1, 2):
        hp = homP(n)
        he = homE(n)
        if he is None:
            continue

        delta = hp - he
        flag = ""

        # Leontovich condition is P_n > E_n (Delta > 0)
        if delta > 0:
            if crossover_n is None:
                crossover_n = n
            flag = "<<< STRONG LEONTOVICH"

        print(f"{n:4} {hp:25} {he:25} {delta:25} {flag:10}")


if __name__ == "__main__":
    # Test the new 242-vertex record: T-hat(1, 12, 1, 18)
    verify_strong_leontovich(1, 12, 1, 18)
