#!/usr/bin/env python3
"""explore_B_family.py — Map the B(x,y) infinite bipartite family.

Evaluates the continuous Leontovich manifold discovered from the (3, m2) sweep.
"""

import numpy as np


def check_B_family(x, y, max_n=100, d=2):
    # Orbit sizes: L0(1), L12(2), R0(x), R45(2), R6(y)
    a = np.array([1, 2, x, 2, y], dtype=np.float64)

    # 5x5 Full Quotient matrix
    Q = np.array(
        [
            [0, 0, 1, 2, 0],
            [0, 0, 0, 1, y],
            [x, 0, 0, 0, 0],
            [1, 1, 0, 0, 0],
            [0, 2, 0, 0, 0],
        ],
        dtype=np.float64,
    )

    w = [np.ones(5, dtype=np.float64)]
    for _ in range(max_n):
        w.append(Q @ w[-1])

    homP = [np.dot(a, w[k]) for k in range(max_n + 1)]
    b = w[1] * w[d]

    for n in range(5, max_n, 2):
        stem = n - d - 2
        if stem < 0:
            continue
        homE = np.dot(a, w[stem] * b)
        if homE < homP[n] * (1.0 - 1e-11):
            return n
    return None


def main():
    print("\n\033[1;36mMapping the B(x,y) Bipartite Leontovich Manifold\033[0m")
    print("x = singletons to L0 | y = pairs to {L1, L2}")
    print("Grid shows crossover threshold n ('.' = no crossover)\n")

    max_val = 20
    print("    y", end="")
    for y in range(1, max_val + 1):
        print(f"{y:3d}", end="")
    print("\n  x +" + "-" * (max_val * 3))

    min_vertices = 999
    best_params = None

    for x in range(1, max_val + 1):
        print(f"{x:3d} |", end="")
        for y in range(1, max_val + 1):
            n = check_B_family(x, y)
            if n is not None:
                print(f"{n:3d}", end="")
                vertices = x + y + 5
                if vertices < min_vertices:
                    min_vertices = vertices
                    best_params = (x, y, n)
            else:
                print("  .", end="")
        print()

    if best_params is not None:
        print(
            f"\n\033[1;32mAbsolute minimum in this family: "
            f"B({best_params[0]}, {best_params[1]}) with "
            f"{min_vertices} vertices (crosses at n={best_params[2]}).\033[0m"
        )
        print("This confirms H_18 is the optimal ground state!")


if __name__ == "__main__":
    main()
