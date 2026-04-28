#!/usr/bin/env python3
"""Verify Leontovich crossover for spherically symmetric trees T(d1,...,dk).

Uses exact integer arithmetic on the quotient (similarity) matrix.
Checks E_n^{(d)} against P_n for d=2..max_d.

Usage:
    # Verify a single tree
    python3 scripts/verify_leontovich.py 7 1 9

    # Sweep all symmetric trees with |V| < V_max
    python3 scripts/verify_leontovich.py --sweep 78
    python3 scripts/verify_leontovich.py --sweep 78 --max-depth 6
"""

import sys
import time


def build_quotient(degrees):
    """Build the (k+1)x(k+1) quotient matrix and orbit sizes."""
    dim = len(degrees) + 1
    M = [[0] * dim for _ in range(dim)]
    for i, d in enumerate(degrees):
        M[i][i + 1] = d
        M[i + 1][i] = 1
    # Orbit sizes
    a = [1]
    for d in degrees:
        a.append(a[-1] * d)
    total_v = sum(a)
    return M, a, dim, total_v


def matvec(M, v, dim):
    """Matrix-vector multiply (exact integers)."""
    return [sum(M[i][j] * v[j] for j in range(dim)) for i in range(dim)]


def check_leontovich(degrees, max_n=200, max_d=20):
    """Check if T(degrees) is Leontovich. Returns (d, n) of first crossover or None."""
    M, a, dim, total_v = build_quotient(degrees)

    # Compute w[k] = M^k * 1 using exact integers
    ones = [1] * dim
    w = [ones]
    for k in range(max_n):
        w.append(matvec(M, w[-1], dim))

    # hom(P_n) = sum_i a[i] * w[n-1][i]
    def homP(n):
        return sum(a[i] * w[n - 1][i] for i in range(dim))

    # Check each depth d
    for d in range(2, min(max_d, max_n) + 1):
        for n in range(d + 3, max_n + 2):
            stem = n - d - 2
            if stem < 0:
                continue
            he = sum(a[i] * w[stem][i] * w[1][i] * w[d][i] for i in range(dim))
            hp = homP(n)
            if hp - he > 0:
                return d, n
    return None


def verify(degrees, max_n=200):
    """Verbose verification of a single tree."""
    M, a, dim, total_v = build_quotient(degrees)

    label = "T(" + ",".join(str(d) for d in degrees) + ")"
    print(f"=== Verifying {label} ===")
    print(f"|V| = {total_v}, orbits = {dim}, orbit sizes = {a}")
    print()

    # Compute w[k] = M^k * 1 using exact integers
    ones = [1] * dim
    w = [ones]
    for k in range(max_n):
        w.append(matvec(M, w[-1], dim))

    # hom(P_n) = sum_i a[i] * w[n-1][i]
    def homP(n):
        return sum(a[i] * w[n - 1][i] for i in range(dim))

    def homE(n, d=2):
        stem = n - d - 2
        if stem < 0:
            return None
        return sum(a[i] * w[stem][i] * w[1][i] * w[d][i] for i in range(dim))

    # Check d=2 crossover
    print(f"{'n':>4} {'hom(P_n)':>30} {'hom(E_n)':>30} {'Delta':>15} {'flag':>10}")
    print("-" * 95)

    crossover_n = None
    for n in range(5, max_n + 2):
        hp = homP(n)
        he = homE(n, d=2)
        if he is None:
            continue
        delta = hp - he
        flag = ""
        if delta > 0 and crossover_n is None:
            crossover_n = n
            flag = "<<< CROSS"
        elif delta > 0:
            flag = "<<<" if n <= crossover_n + 10 else ""
        # Only print interesting lines
        if (
            n <= 25
            or delta > 0
            and n <= (crossover_n or 999) + 10
            or n % 2 == 1
            and n <= 51
        ):
            print(f"{n:4d} {str(hp):>30s} {str(he):>30s} {str(delta):>15s} {flag:>10s}")

    print()
    if crossover_n is not None:
        print(f"✓ VERIFIED: {label} is Leontovich at d=2!")
        print(f"  First crossover at n = {crossover_n} (odd)")
        print(f"  |V| = {total_v}")
        print(f"  Delta(n={crossover_n}) = {homP(crossover_n) - homE(crossover_n)}")
    else:
        print(f"✗ No d=2 crossover found for {label} up to n = {max_n}")

    # Also check higher depths
    print()
    print("--- Higher depth check (d=3..10) ---")
    for d in range(3, 11):
        for n in range(d + 3, max_n + 2):
            stem = n - d - 2
            if stem < 0:
                continue
            he_d = sum(a[i] * w[stem][i] * w[1][i] * w[d][i] for i in range(dim))
            hp_n = homP(n)
            if hp_n - he_d > 0:
                print(f"  d={d}: first crossover at n={n}, Delta = {hp_n - he_d}")
                break
        else:
            print(f"  d={d}: no crossover up to n={max_n}")

    return crossover_n


def enumerate_trees(max_v, depth):
    """Generate all branching sequences of given depth with |V| <= max_v."""
    if depth == 0:
        yield ()
        return
    for d1 in range(1, max_v):
        if 1 + d1 > max_v:
            break
        if depth == 1:
            yield (d1,)
        else:
            for rest in enumerate_trees_inner(max_v, depth - 1, d1, 1 + d1):
                yield (d1,) + rest


def enumerate_trees_inner(max_v, depth, prev_orbit_size, current_v):
    """Recursively enumerate branching sequences."""
    for d in range(1, max_v):
        new_orbit = prev_orbit_size * d
        new_v = current_v + new_orbit
        if new_v > max_v:
            break
        if depth == 1:
            yield (d,)
        else:
            for rest in enumerate_trees_inner(max_v, depth - 1, new_orbit, new_v):
                yield (d,) + rest


def sweep(max_v, max_depth=6, max_n=200, max_d=20):
    """Sweep all symmetric trees with |V| <= max_v using exact arithmetic."""
    print(f"=== Exact Leontovich Sweep: |V| <= {max_v}, depth <= {max_depth} ===")
    print(f"Parameters: max_n={max_n}, max_d={max_d}")
    print()

    hits = []
    per_depth = {}
    t0 = time.time()
    total = 0

    for depth in range(1, max_depth + 1):
        count = 0
        leo_count = 0
        for degrees in enumerate_trees(max_v, depth):
            _, _, _, total_v = build_quotient(degrees)
            if total_v > max_v:
                continue
            count += 1
            total += 1

            result = check_leontovich(degrees, max_n=max_n, max_d=max_d)
            if result is not None:
                d_cross, n_cross = result
                leo_count += 1
                label = "T(" + ",".join(str(x) for x in degrees) + ")"
                hits.append((total_v, depth, label, d_cross, n_cross))
                print(
                    f"  ✓ {label} |V|={total_v} d={d_cross} n>={n_cross}",
                    flush=True,
                )

            if count % 500 == 0:
                elapsed = time.time() - t0
                print(
                    f"  [depth {depth}] {count} checked, {leo_count} hits"
                    f" ({elapsed:.1f}s)",
                    flush=True,
                )

        per_depth[depth] = (count, leo_count)
        elapsed = time.time() - t0
        print(
            f"  depth {depth} ({depth+1} orbits): {count} trees,"
            f" {leo_count} Leontovich ({elapsed:.1f}s)"
        )

    elapsed = time.time() - t0
    print()
    print(f"{'Depth':>6} {'Orbits':>7} {'Tested':>8} {'Leontovich':>12}")
    print("-" * 40)
    total_tested = 0
    total_leo = 0
    for depth in sorted(per_depth):
        tested, leo_count = per_depth[depth]
        total_tested += tested
        total_leo += leo_count
        print(f"{depth:>6} {depth+1:>7} {tested:>8,} {leo_count:>12}")
    print("-" * 40)
    print(f"{'Total':>14} {total_tested:>8,} {total_leo:>12}")
    print(f"\nElapsed: {elapsed:.1f}s")

    if hits:
        print(f"\n=== {len(hits)} Leontovich instances found ===")
        hits.sort()
        for v, dep, label, dc, nc in hits:
            print(f"  {label:>25s}  |V|={v:>5d}  d={dc}  n>={nc}")
    else:
        print("\n=== No Leontovich instances found ===")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python3 verify_leontovich.py d1 d2 ... dk     # verify single tree")
        print("  python3 verify_leontovich.py --sweep V_MAX    # sweep all |V|<=V_MAX")
        print("  python3 verify_leontovich.py --sweep 78 --max-depth 6")
        sys.exit(1)

    if sys.argv[1] == "--sweep":
        max_v = int(sys.argv[2])
        max_depth = 6
        for i, arg in enumerate(sys.argv):
            if arg == "--max-depth" and i + 1 < len(sys.argv):
                max_depth = int(sys.argv[i + 1])
        sweep(max_v, max_depth=max_depth)
    else:
        degrees = [int(x) for x in sys.argv[1:]]
        verify(degrees)
