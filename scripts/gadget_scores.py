#!/usr/bin/env python3
"""Evaluate exact near-path signs and the long-spider Perron score."""

from __future__ import annotations

import argparse

import numpy as np
from exact_rho import certify_rho_sign, symmetric_tree_quotient


def perron_data(quotient, sizes):
    """Return the Perron root and right eigenvector of a balanced quotient."""
    q = np.asarray(quotient, dtype=float)
    weights = np.asarray(sizes, dtype=float)
    root_weights = np.sqrt(weights)
    symmetric = root_weights[:, None] * q / root_weights[None, :]
    values, vectors = np.linalg.eigh(symmetric)
    eigenvector = vectors[:, -1] / root_weights
    if eigenvector[0] < 0:
        eigenvector = -eigenvector
    return float(values[-1]), eigenvector


def near_path_ratio(quotient, sizes, depth):
    """Return the numerical Perron coefficient ratio rho_depth."""
    q = np.asarray(quotient, dtype=float)
    weights = np.asarray(sizes, dtype=float)
    lam, eigenvector = perron_data(quotient, sizes)
    ones = np.ones(len(sizes))
    w1 = q @ ones
    wd = np.linalg.matrix_power(q, depth) @ ones
    numerator = np.sum(weights * eigenvector * w1 * wd)
    denominator = lam ** (depth + 1) * np.sum(weights * eigenvector)
    return float(numerator / denominator)


def long_spider_score(quotient, sizes):
    """Return the limiting ratio for a spider with three long legs."""
    weights = np.asarray(sizes, dtype=float)
    _, eigenvector = perron_data(quotient, sizes)
    first = np.sum(weights * eigenvector)
    second = np.sum(weights * eigenvector**2)
    third = np.sum(weights * eigenvector**3)
    return float(first * third / second**2)


def main():
    """Print scores and exact near-path sign certificates for a radial target."""
    parser = argparse.ArgumentParser()
    parser.add_argument("degrees", nargs="+", type=int)
    parser.add_argument("--max-depth", type=int, default=8)
    args = parser.parse_args()
    quotient, sizes = symmetric_tree_quotient(args.degrees)
    print(f"long-spider score tau = {long_spider_score(quotient, sizes):.12f}")
    for depth in range(1, args.max_depth + 1):
        ratio = near_path_ratio(quotient, sizes, depth)
        relation = certify_rho_sign(quotient, sizes, depth).relation
        print(f"rho_{depth} = {ratio:.12f} ({relation} 1 exactly)")


if __name__ == "__main__":
    main()
