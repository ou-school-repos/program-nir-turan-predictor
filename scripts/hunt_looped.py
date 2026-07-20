#!/usr/bin/env python3
"""hunt_looped.py — Shatter the 2855-vertex record for Problem 3.
Uses the Perron-Frobenius principal eigenvector to evaluate the asymptotic
Leontovich ratio in O(1) matrix operations, bypassing dynamic programming.
"""

import time

import numpy as np


def check_looped_ratio(degrees, d=2):
    """Check ratio to detect matches."""

    k = len(degrees)
    dim = k + 1
    Q = np.zeros((dim, dim))
    Q[0, 0] = 1.0  # The Loop at the Root
    for i in range(k):
        Q[i, i + 1] = degrees[i]
        Q[i + 1, i] = 1.0

    a = [1.0]
    for deg in degrees:
        a.append(a[-1] * deg)
    a = np.array(a)

    # Exact 1-step and d-step walks
    w1 = Q @ np.ones(dim)
    wd = np.ones(dim)
    for _ in range(d):
        wd = Q @ wd

    # Principal right eigenvector
    evals, evecs = np.linalg.eig(Q)
    idx = np.argmax(np.real(evals))
    lam1 = np.real(evals[idx])
    v = np.real(evecs[:, idx])
    if v[0] < 0:
        v = -v  # Ensure positivity

    # Asymptotic Ratio R = c_E / c_P
    num = np.sum(a * v * w1 * wd)
    den = (lam1 ** (d + 1)) * np.sum(a * v)
    return num / den


def main():
    print(
        "\n\033[1;36mHunting Minimal Looped Strongly Leontovich Graphs (Problem 3)\033[0m"
    )
    print("Previous Record: T_loop(2, 31, 1, 44) | V = 2,855\n")

    best_v = 2855
    best_graph = (2, 31, 1, 44)
    t0 = time.time()

    def hunt(depth, current_degrees, current_V, current_term, b_v, b_g):
        if len(current_degrees) == depth:
            R = check_looped_ratio(current_degrees)
            if R < 1.0:
                b_v = current_V
                b_g = current_degrees
                deg_str = ",".join(map(str, current_degrees))
                print(
                    f"\033[1;32m  >>> NEW RECORD! T_loop({deg_str}) "
                    f"| Vertices: {current_V} | R: {R:.5f}\033[0m"
                )
            return b_v, b_g

        for deg in range(1, b_v):
            next_term = current_term * deg
            next_V = current_V + next_term
            if next_V >= b_v:
                break
            b_v, b_g = hunt(
                depth,
                current_degrees + (deg,),
                next_V,
                next_term,
                b_v,
                b_g,
            )
        return b_v, b_g

    # Sweep depths (number of orbits)
    for depth in range(3, 8):
        print(f"Sweeping Depth {depth}...")
        best_v, best_graph = hunt(depth, (), 1, 1, best_v, best_graph)

    best_str = ",".join(map(str, best_graph)) if best_graph else "None"
    print(
        f"\n\033[1;33mSmallest floating-point candidate in the depth-3..7 sweep: "
        f"T_loop({best_str}) with {best_v} vertices!\033[0m"
    )
    print(f"Elapsed: {time.time() - t0:.2f}s")


if __name__ == "__main__":
    main()
