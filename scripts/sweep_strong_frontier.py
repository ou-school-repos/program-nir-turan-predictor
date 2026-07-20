#!/usr/bin/env python3
"""Sweep looped symmetric trees with the Perron coefficient criterion.

The sweep enumerates looped symmetric trees T-hat(d1,...,dk), 2 <= k <= 6,
with vertex count at most CAP.  NumPy is used only as a fast prescreen; every
reported rho < 1 candidate is recomputed with scripts/strong_coeff.py.
"""

from __future__ import annotations

import argparse
import json
import time

import numpy as np
from strong_coeff import leading_ratio


def vertex_count(degrees: tuple[int, ...]) -> int:
    """Return the total vertex count of T-hat(degrees)."""
    total = 1
    prod = 1
    for degree in degrees:
        prod *= degree
        total += prod
    return total


def rho_float(degrees: tuple[int, ...]) -> float:
    """Fast float prescreen for the leading-coefficient ratio."""
    dim = len(degrees) + 1
    q = np.zeros((dim, dim), dtype=float)
    q[0, 0] = 1.0
    for i, degree in enumerate(degrees):
        q[i, i + 1] = float(degree)
        q[i + 1, i] = 1.0

    sizes = np.array([1.0] + list(np.cumprod(degrees)), dtype=float)
    w1 = q @ np.ones(dim)
    w2 = q @ w1
    values, vectors = np.linalg.eig(q)
    idx = int(np.argmax(values.real))
    lam = float(values[idx].real)
    u = vectors[:, idx].real
    if u[0] < 0:
        u = -u

    numerator = float((sizes * u * w1 * w2).sum())
    denominator = (lam**3) * float((sizes * u).sum())
    return numerator / denominator


def sweep(cap: int, max_levels: int, tolerance: float):
    """Enumerate candidates and certify rho < 1 hits."""
    count = 0
    candidates = []
    near_best = (float("inf"), None)

    def rec(degrees: list[int], vertices: int, prod: int) -> None:
        nonlocal count, near_best
        if len(degrees) >= 2:
            params = tuple(degrees)
            count += 1
            rho = rho_float(params)
            if rho < near_best[0]:
                near_best = (rho, (vertices, params))
            if rho < 1.0 - tolerance:
                certified = leading_ratio(params)
                if certified < 1.0:
                    candidates.append((vertices, params, certified))

        if len(degrees) == max_levels:
            return

        degree = 1
        while True:
            next_vertices = vertices + prod * degree
            if next_vertices > cap:
                break
            degrees.append(degree)
            rec(degrees, next_vertices, prod * degree)
            degrees.pop()
            degree += 1

    rec([], 1, 1)
    candidates.sort()
    return count, candidates, near_best


def main() -> None:
    """Run the command-line frontier sweep."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--cap", type=int, default=4200)
    parser.add_argument("--max-levels", type=int, default=6)
    parser.add_argument("--tolerance", type=float, default=1e-10)
    parser.add_argument("--top", type=int, default=12)
    parser.add_argument("--json-out", default="cands.json")
    args = parser.parse_args()

    start = time.time()
    count, candidates, near_best = sweep(args.cap, args.max_levels, args.tolerance)
    elapsed = time.time() - start

    print(
        f"swept {count} looped symmetric trees "
        f"(2-{args.max_levels} branching levels, V<={args.cap}) in {elapsed:.0f}s"
    )
    print(f"certified candidates with rho < 1: {len(candidates)}")
    print(f"smallest {min(args.top, len(candidates))} by |V|:")
    for vertices, degrees, rho in candidates[: args.top]:
        print(f"  V={vertices:5d} T^{degrees} rho={rho:.12f}")
    if near_best[1] is not None:
        rho, (vertices, degrees) = near_best
        print(f"best prescreen rho: V={vertices} T^{degrees} rho_float={rho:.12f}")

    with open(args.json_out, "w") as fh:
        json.dump([[v, list(d), rho] for v, d, rho in candidates], fh)


if __name__ == "__main__":
    main()
