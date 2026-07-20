#!/usr/bin/env python3
"""
Depth-5 sweep: find the threshold landscape of smallest Leontovich graphs
among all spherically symmetric trees T(d1, d2, ..., dk) for k ≤ 5.

Uses the O(1) similarity matrix trick: for a tree with branching
sequence D = [d1, ..., dk], the quotient matrix M is (k+1)×(k+1)
tridiagonal. We compute eigenvalues of M and use the orbit-weighted
transfer matrix to evaluate hom(P_n, H) and hom(E_n, H) in O(k²)
per n, avoiding the full |V|×|V| adjacency matrix entirely.

Usage:
    python3 scripts/depth5_sweep.py [--max-vertices 400] [--max-degree 20]
"""

import argparse
import sys

import numpy as np


def get_similarity_matrix(degrees):
    """
    Build similarity matrix M and orbit sizes for symmetric tree T(d1, ..., dk).

    M is (k+1)×(k+1) tridiagonal:
      M[i, i+1] = d_{i+1}  (parent → children)
      M[i+1, i] = 1        (child → parent)

    Orbit sizes: a = [1, d1, d1*d2, d1*d2*d3, ...]
    Total vertices: sum(a)
    """
    k = len(degrees)
    dim = k + 1
    M = np.zeros((dim, dim))
    for i in range(k):
        M[i, i + 1] = degrees[i]
        M[i + 1, i] = 1

    a = [1]
    for d in degrees:
        a.append(a[-1] * d)

    return M, a


def check_leontovich_similarity(M, a, max_n=300):
    """
    Check Leontovich property using the similarity matrix.

    We compute w[step] = M^step · 1_weighted, where the initial vector
    is the orbit sizes (since each orbit contributes a[i] identical vertices).

    Returns (is_leo, first_n, first_d, ratio, lam1, lam2) or None.
    """
    dim = len(a)
    total = sum(a)

    # Weighted walk counts: w[step][i] = (A^step · 1)[any vertex in orbit i]
    # homP[n] = sum_i a[i] * w[n-1][i]
    # For the similarity matrix, we iterate w = M @ w but need to account
    # for the orbit weighting properly.
    #
    # The trick: let v[0] = vector of 1s (one per orbit).
    # Then v[step] = M^step @ v[0].
    # hom(P_n, H) = sum_i a[i] * v[n-1][i]
    #
    # For E_n with depth d: hom(E_n, H) = sum_i a[i] * v[stem][i] * b[i]
    # where b[i] = v[1][i] * v[d][i] (the branch contribution)

    v = np.zeros((max_n, dim))
    v[0] = np.ones(dim)
    a_arr = np.array(a, dtype=np.float64)

    homP = np.zeros(max_n + 1)
    homP[1] = total

    for step in range(1, max_n):
        v[step] = M @ v[step - 1]
        homP[step + 1] = np.dot(a_arr, v[step])

    # Eigenvalues for reporting
    evals = sorted(np.linalg.eigvalsh(M), reverse=True)
    lam1 = evals[0] if len(evals) > 0 else 0
    lam2 = evals[1] if len(evals) > 1 else 0

    # Check crossovers for each depth d
    for d in range(2, min(21, max_n - 2)):
        b = v[1] * v[d]
        for stem in range(0, max_n - d - 2):
            homE = np.dot(a_arr, v[stem] * b)
            n = stem + d + 2
            if n >= max_n:
                break
            if homP[n] > 0 and homE / homP[n] < 1.0 - 1e-11:
                ratio = homE / homP[n]
                return True, n, d, ratio, lam1, lam2

    return False, None, None, None, lam1, lam2


def enumerate_branching_sequences(max_depth, max_degree, max_vertices):
    """
    Yield all branching sequences [d1, ..., dk] for k=1..max_depth
    where each di ∈ [1, max_degree] and total vertices ≤ max_vertices.
    """
    for depth in range(1, max_depth + 1):
        # Generate sequences with pruning
        yield from _enumerate_recursive(depth, max_degree, max_vertices, [])


def _enumerate_recursive(remaining, max_degree, max_vertices, current):
    """Recursively enumerate branching sequences with early pruning."""
    if remaining == 0:
        yield tuple(current)
        return

    # Current vertex count
    orbit_product = 1
    for d in current:
        orbit_product *= d
    current_vertices = sum(_prefix_product(current, i) for i in range(len(current) + 1))

    for d in range(1, max_degree + 1):
        # New orbit adds orbit_product * d vertices
        new_vertices = current_vertices + orbit_product * d
        if new_vertices > max_vertices:
            break
        current.append(d)
        yield from _enumerate_recursive(
            remaining - 1, max_degree, max_vertices, current
        )
        current.pop()


def _prefix_product(seq, length):
    """Product of first `length` elements of seq."""
    p = 1
    for i in range(length):
        p *= seq[i]
    return p


def main():
    parser = argparse.ArgumentParser(
        description="Sweep spherically symmetric trees up to depth 5"
    )
    parser.add_argument(
        "--max-vertices",
        type=int,
        default=400,
        help="Maximum vertex count (default: 400)",
    )
    parser.add_argument(
        "--max-degree",
        type=int,
        default=20,
        help="Maximum branching degree (default: 20)",
    )
    parser.add_argument(
        "--max-depth",
        type=int,
        default=5,
        help="Maximum tree depth / branching sequence length (default: 5)",
    )
    args = parser.parse_args()

    print(f"=== Depth-{args.max_depth} Leontovich Sweep ===")
    print(f"Max vertices: {args.max_vertices}, Max degree: {args.max_degree}")
    print()

    # Track: for each threshold n, the smallest |V| Leontovich graph
    frontier = {}  # threshold_n -> (|V|, degrees, d, ratio, lam1, lam2)
    total_checked = 0
    total_leo = 0

    for degrees in enumerate_branching_sequences(
        args.max_depth, args.max_degree, args.max_vertices
    ):
        total_checked += 1

        M, a = get_similarity_matrix(degrees)
        total_v = sum(a)

        is_leo, first_n, first_d, ratio, lam1, lam2 = check_leontovich_similarity(M, a)

        if is_leo:
            total_leo += 1
            if first_n not in frontier or total_v < frontier[first_n][0]:
                frontier[first_n] = (total_v, degrees, first_d, ratio, lam1, lam2)
                print(
                    f"  NEW BEST n≥{first_n}: T{degrees} |V|={total_v} "
                    f"d={first_d} r={ratio:.8f} λ1={lam1:.4f} λ2={lam2:.4f}"
                )

        if total_checked % 10000 == 0:
            print(
                f"  ... checked {total_checked} sequences, "
                f"found {total_leo} Leontovich ...",
                file=sys.stderr,
            )

    print(f"\n{'=' * 60}")
    print(f"Checked {total_checked} branching sequences, found {total_leo} Leontovich")
    print(f"\n{'=' * 60}")
    print("THRESHOLD LANDSCAPE: Smallest Leontovich graph per threshold n")
    print(f"{'=' * 60}")
    print(
        f"{'n':>4s}  {'|V|':>6s}  {'Structure':25s}  {'d':>3s}  {'λ1':>8s}  {'λ2':>8s}  {'ratio':>12s}"
    )
    print("-" * 72)

    for n in sorted(frontier.keys()):
        v, deg, d, ratio, lam1, lam2 = frontier[n]
        deg_str = f"T{deg}"
        print(
            f"{n:4d}  {v:6d}  {deg_str:25s}  {d:3d}  {lam1:8.4f}  {lam2:8.4f}  {ratio:12.8f}"
        )

    # Also find absolute minimum
    if frontier:
        min_n, (min_v, min_deg, min_d, min_r, min_l1, min_l2) = min(
            frontier.items(), key=lambda x: x[1][0]
        )
        print(f"\nAbsolute minimum: T{min_deg} with |V|={min_v} at n≥{min_n}")


if __name__ == "__main__":
    main()
