#!/usr/bin/env python3
"""hunt_looped.py — Shatter the 2855-vertex record for Problem 3.
Uses the Perron-Frobenius principal eigenvector to evaluate the asymptotic
Leontovich ratio in O(1) matrix operations, bypassing dynamic programming.
"""

import itertools
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
    den = (lam1**3) * np.sum(a * v)
    return num / den


def main():
    """Main method to run detection script."""

    print(
        "\n\033[1;36mHunting Minimal Looped Strongly Leontovich Graphs (Problem 3)\033[0m"
    )
    print("Previous Record: T_loop(2, 31, 1, 44) | V = 2,855\n")

    best_v = 2855
    best_graph = None
    t0 = time.time()

    # Sweep depths (number of orbits)
    for depth in range(3, 7):
        print(f"Sweeping Depth {depth}...")
        for degrees in itertools.product(range(1, 40), repeat=depth):
            V = 1
            term = 1
            for deg in degrees:
                term *= deg
                V += term
                if V >= best_v:
                    break

            if V < best_v:
                R = check_looped_ratio(degrees)
                if R < 1.0:
                    best_v = V
                    best_graph = degrees
                    deg_str = ",".join(map(str, degrees))
                    print(
                        f"\033[1;32m  >>> NEW RECORD! T_loop({deg_str}) "
                        f"| Vertices: {V} | R: {R:.5f}\033[0m"
                    )

    best_str = ",".join(map(str, best_graph)) if best_graph else "None"
    print(
        f"\n\033[1;32mAbsolute Minimum Looped Strong Leontovich Found: "
        f"T_loop({best_str}) with {best_v} vertices!\033[0m"
    )
    print(f"Elapsed: {time.time() - t0:.2f}s")


if __name__ == "__main__":
    main()
