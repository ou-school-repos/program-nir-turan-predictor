#!/usr/bin/env python3
"""analyze_near_miss.py — Trace and analyze our closest non-trivial near-miss."""

from find_near_misses import get_min_ratio


def main():
    # Top near miss vector
    c_vec = (9, 1, 0, 0, 0, 1, 0)
    print(f"\nTracing exact ratios for near-miss graph {c_vec} (m = 14 vertices):")
    print("=" * 60)
    print(" Threshold n  |  Ratio hom(E_n^(2), H) / hom(P_n, H)")
    print("=" * 60)
    for n in range(5, 53, 2):
        ratio = get_min_ratio(c_vec, n=n)
        diff_pct = (ratio - 1.0) * 100
        marker = " (STRICTLY > 1.0)" if ratio > 1.0 else " (CROSSOVER! < 1.0)"
        print(f"    n = {n:2d}      |  {ratio:.10f} ({diff_pct:+.8f}%){marker}")
    print("=" * 60)


if __name__ == "__main__":
    main()
