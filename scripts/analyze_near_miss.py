#!/usr/bin/env python3
"""analyze_near_miss.py — Trace and analyze our closest near-misses (both disconnected and connected)."""

from find_near_misses import get_min_ratio


def main():
    # 1. Disconnected near-miss vector (asymptotic dilution)
    c_dis = (9, 1, 0, 0, 0, 1, 0)
    print(
        f"\n1. Tracing disconnected near-miss graph {c_dis} (asymptotic dilution, m = 14 vertices):"
    )
    print("=" * 65)
    print(" Threshold n  |  Ratio hom(E_n^(2), H) / hom(P_n, H)")
    print("=" * 65)
    for n in range(5, 33, 2):
        ratio = get_min_ratio(c_dis, n=n)
        diff_pct = (ratio - 1.0) * 100
        marker = " (STRICTLY > 1.0)" if ratio > 1.0 else " (CROSSOVER! < 1.0)"
        print(f"    n = {n:2d}      |  {ratio:.10f} ({diff_pct:+.8f}%){marker}")
    print("=" * 65)

    # 2. Connected near-miss vector
    c_con = (6, 0, 0, 1, 1, 5, 0)
    print(
        f"\n2. Tracing closest connected near-miss from the (3, m2 <= 15), n = 17 "
        f"search {c_con} (m = 16 vertices):"
    )
    print("=" * 65)
    print(" Threshold n  |  Ratio hom(E_n^(2), H) / hom(P_n, H)")
    print("=" * 65)
    for n in range(5, 33, 2):
        ratio = get_min_ratio(c_con, n=n)
        diff_pct = (ratio - 1.0) * 100
        marker = " (STRICTLY > 1.0)" if ratio > 1.0 else " (CROSSOVER! < 1.0)"
        print(f"    n = {n:2d}      |  {ratio:.10f} ({diff_pct:+.8f}%){marker}")
    print("=" * 65)


if __name__ == "__main__":
    main()
