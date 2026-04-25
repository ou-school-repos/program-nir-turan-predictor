#!/usr/bin/env python3
"""Transfer matrix analysis for hom(P_n, P_k) baselines."""

import numpy as np

for k in [3, 5, 7, 9]:
    # Adjacency matrix of P_k
    A = np.zeros((k, k))
    for i in range(k - 1):
        A[i][i + 1] = 1
        A[i + 1][i] = 1

    # Eigenvalues
    eigs = sorted(np.linalg.eigvalsh(A), reverse=True)
    print(f"\n=== P_{k} (adjacency matrix {k}x{k}) ===")
    print(f"Eigenvalues: {[f'{e:.6f}' for e in eigs]}")
    print(f"Dominant: {eigs[0]:.6f}")

    # Characteristic polynomial coefficients
    coeffs = np.round(np.poly(A)).astype(int)
    terms = []
    for i, c in enumerate(coeffs):
        if c != 0:
            terms.append(f"{c:+d}x^{k-i}")
    print(f"Char poly: {' '.join(terms)}")

    # Compute hom(P_n, P_k) for n=1..20
    v = np.ones(k)
    vals = []
    for n in range(1, 21):
        vals.append(int(round(np.sum(v))))
        v = A @ v

    print(f"hom(P_n, P_{k}) for n=1..20:")
    for n in [5, 10, 15, 20]:
        print(f"  n={n}: {vals[n-1]}")

    # Growth ratio
    for n in [15, 20]:
        ratio = vals[n - 1] / vals[n - 2]
        print(f"  ratio s_{n}/s_{n-1} = {ratio:.6f} (→ {eigs[0]:.6f})")
