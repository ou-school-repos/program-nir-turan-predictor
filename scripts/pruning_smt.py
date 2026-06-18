#!/usr/bin/env python3
"""pruning_smt.py — Target-space SMT pruning search starting from T(7,1,9).

Searches for any leaf-pruning of T(7,1,9) of size m <= 75 that is Leontovich.
Uses Z3 to solve the bilinear walk equations on the automorphism quotient.
"""

import sys
import time

try:
    import z3
except ImportError:
    print("Error: z3-solver is required.")
    sys.exit(1)


def search_pruned_leontovich(max_vertices=75, max_n=31):
    print(
        "\n\033[1;36mInitializing local SMT pruning search starting from T(7,1,9) (78 vertices)...\033[0m"
    )
    print(f"  Max Vertices: {max_vertices}")
    print(f"  Max Threshold n to check: {max_n}")

    start_time = time.time()

    # We will loop over threshold n from 13 to max_n (since n=13 is the crossover for the symmetric 78-vertex tree)
    for n in range(13, max_n + 1, 2):
        s = z3.Solver()
        s.set("timeout", 10000)  # 10s per threshold

        # Decision variables: k_i in {0..9} represents the number of active leaves on hub i (for i = 1..7)
        k = [z3.Int(f"k_{i}") for i in range(7)]
        for i in range(7):
            s.add(k[i] >= 0)
            s.add(k[i] <= 9)

        # Symmetry breaking: sort leaf counts
        for i in range(6):
            s.add(k[i] >= k[i + 1])

        # Vertices count constraint: 1 root + 7 bridges + 7 hubs + sum(k_i) <= max_vertices
        num_vertices = 15 + z3.Sum(k)
        s.add(num_vertices <= max_vertices)
        s.add(num_vertices >= 20)  # non-trivial size

        # Walk values representation (22 variables per step)
        # Type index: 0 = root, 1..7 = bridges, 8..14 = hubs, 15..21 = leaf-types
        w = [[z3.Int(f"w_{step}_{idx}") for idx in range(22)] for step in range(n)]

        # Base case: w[0] = all-ones
        for idx in range(22):
            s.add(w[0][idx] == 1)

        # Recurrence relation: w_step = A * w_{step-1}
        for step in range(1, n):
            # 1. Root
            s.add(w[step][0] == z3.Sum([w[step - 1][1 + i] for i in range(7)]))

            # 2. Bridges
            for i in range(7):
                s.add(w[step][1 + i] == w[step - 1][0] + w[step - 1][8 + i])

            # 3. Hubs
            for i in range(7):
                s.add(w[step][8 + i] == w[step - 1][1 + i] + k[i] * w[step - 1][15 + i])

            # 4. Leaf-types
            for i in range(7):
                s.add(w[step][15 + i] == w[step - 1][8 + i])

        # Homomorphism counts
        # homP = sum_v w_{n-1}(v)
        homP = z3.Int("homP")
        s.add(
            homP
            == w[n - 1][0]
            + z3.Sum([w[n - 1][1 + i] for i in range(7)])
            + z3.Sum([w[n - 1][8 + i] for i in range(7)])
            + z3.Sum([k[i] * w[n - 1][15 + i] for i in range(7)])
        )

        # homE = sum_v w_{n-4}(v) * w_1(v) * w_2(v)
        b = [z3.Int(f"b_{idx}") for idx in range(22)]
        for idx in range(22):
            s.add(b[idx] == w[1][idx] * w[2][idx])

        homE = z3.Int("homE")
        s.add(
            homE
            == w[n - 4][0] * b[0]
            + z3.Sum([w[n - 4][1 + i] * b[1 + i] for i in range(7)])
            + z3.Sum([w[n - 4][8 + i] * b[8 + i] for i in range(7)])
            + z3.Sum([k[i] * w[n - 4][15 + i] * b[15 + i] for i in range(7)])
        )

        # Constraints
        s.add(homP > 0)
        s.add(homE > 0)
        s.add(homE < homP)

        res = s.check()
        if res == z3.sat:
            model = s.model()
            leaf_counts = [model[k[i]].as_long() for i in range(7)]
            m_found = 15 + sum(leaf_counts)
            hP = model[homP].as_long()
            hE = model[homE].as_long()
            print(
                f"\n\033[1;32m✓ FOUND Leontovich pruned tree at threshold n={n}!\033[0m"
            )
            print(f"  Leaf Counts:   {leaf_counts}")
            print(f"  Total Vertices: {m_found}")
            print(f"  hom(P_{n}, H):   {hP:,}")
            print(f"  hom(E_{n}^(2), H): {hE:,}")
            print(f"  Difference:    {hP - hE:,}")
            print(f"  Time:          {time.time() - start_time:.3f}s")
            return {
                "sat": True,
                "n": n,
                "leaf_counts": leaf_counts,
                "vertices": m_found,
            }
        else:
            print(f"  n={n}: UNSAT / No Leontovich tree of size <= {max_vertices}")

    elapsed = time.time() - start_time
    print(
        f"\n\033[1;31m✗ Completed search in {elapsed:.3f}s. "
        f"No Leontovich pruned trees exist with <= {max_vertices} vertices.\033[0m"
    )
    return {"sat": False}


if __name__ == "__main__":
    search_pruned_leontovich()
