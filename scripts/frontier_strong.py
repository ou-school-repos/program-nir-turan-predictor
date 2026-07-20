#!/usr/bin/env python3
"""Compute the frontier for Strongly Leontovich 5-orbit looped symmetric trees.

A 5-orbit looped tree T-hat(d1, d2, d3, d4) has quotient matrix:
      [1   d1  0   0   0 ]  <-- looped root (degree d1 + 1)
      [1   0   d2  0   0 ]
  Q = [0   1   0   d3  0 ]
      [0   0   1   0   d4]
      [0   0   0   1   0 ]
"""

import time

MAX_ODD_N = 17501


def evaluate_strong_leontovich(d1, d2, d3, d4, max_odd_n=MAX_ODD_N):
    # Total vertices V = 1 + d1 + d1*d2 + d1*d2*d3 + d1*d2*d3*d4
    V = 1 + d1 + d1 * d2 + d1 * d2 * d3 + d1 * d2 * d3 * d4

    # The degree vector h0 is exactly the row sums of Q for symmetric trees
    a = [1, d1, d1 * d2, d1 * d2 * d3, d1 * d2 * d3 * d4]

    # We must use exact integer arithmetic for the walks to be perfectly accurate
    Q_int = [
        [1, d1, 0, 0, 0],
        [1, 0, d2, 0, 0],
        [0, 1, 0, d3, 0],
        [0, 0, 1, 0, d4],
        [0, 0, 0, 1, 0],
    ]

    w = [[1, 1, 1, 1, 1]]

    def extend_walks(target):
        while len(w) <= target:
            prev = w[-1]
            nxt = [0, 0, 0, 0, 0]
            for i in range(5):
                nxt[i] = sum(Q_int[i][j] * prev[j] for j in range(5))
            w.append(nxt)

    def homP(n):
        return sum(a[i] * w[n - 1][i] for i in range(5))

    def homE(n):
        # E_n^(2) at depth d=2
        # center vertex connects to a path of length (n-2-2) = n-4, a path of length 1, and a path of length 2
        stem = n - 4
        if stem < 0:
            return 0
        return sum(a[i] * w[stem][i] * w[1][i] * w[2][i] for i in range(5))

    threshold = -1

    for n in range(5, max_odd_n + 1, 2):
        extend_walks(n - 1)
        positive = homP(n) > homE(n)
        if positive and threshold == -1:
            threshold = n
        if threshold != -1 and not positive:
            return False, V, -1

    return threshold != -1, V, threshold


def main():
    print("Sweeping 5-orbit looped symmetric trees T-hat(d1,d2,d3,d4)...")
    frontier = {}

    start = time.time()
    count = 0
    # Targeted grid search focusing around the known record (2, 31, 1, 44)
    for d1 in range(1, 5):
        for d2 in range(1, 40):
            for d3 in range(1, 5):
                for d4 in range(1, 60):
                    count += 1
                    is_strong, V, threshold = evaluate_strong_leontovich(d1, d2, d3, d4)
                    if is_strong:
                        if threshold not in frontier or V < frontier[threshold][0]:
                            frontier[threshold] = (V, d1, d2, d3, d4)
                            print(
                                f"New Best for n={threshold}: V={V} at ({d1},{d2},{d3},{d4})"
                            )

    print(f"\nCompleted {count} evaluations in {time.time()-start:.2f}s")
    print("\n--- LaTeX Table ---")
    print("\\begin{table}[H]")
    print("\\centering")
    print(
        "\\caption{Frontier of minimal finite-window Leontovich candidates "
        "within the targeted grid, verified for odd "
        f"$n \\le {MAX_ODD_N}$.}}"
    )
    print("\\label{tab:strong_frontier}")
    print("\\begin{tabular}{@{}ccl@{}}")
    print("  \\toprule")
    print("  Threshold $n$ & $|V|$ & Parameters $(d_1, d_2, d_3, d_4)$ \\\\")
    print("  \\midrule")
    for n in sorted(frontier.keys()):
        v, d1, d2, d3, d4 = frontier[n]
        print(f"  {n} & {v:,} & $\\hat{{T}}({d1}, {d2}, {d3}, {d4})$ \\\\")
    print("  \\bottomrule")
    print("\\end{tabular}")
    print("\\end{table}")


if __name__ == "__main__":
    main()
