#!/usr/bin/env python3
"""Analyze the d<=3 constrained extremal sequence for recurrence relations."""

import math

import numpy as np

# Verified values from exhaustive enumeration
seq = [
    # N=3..22
    5,
    9,
    14,
    24,
    41,
    66,
    110,
    189,
    305,
    510,
    863,
    1425,
    2345,
    3987,
    6515,
    10905,
    18254,
    30135,
    49913,
    84546,
]

print("=" * 60)
print("Δ≤3 Constrained Extremal Sequence (N=3..22)")
print("=" * 60)
print(f"\nSequence: {','.join(str(x) for x in seq)}")

# Growth ratios
print("\n--- Growth Ratios a(n)/a(n-1) ---")
for i in range(1, len(seq)):
    print(f"  N={i+3:2d}: {seq[i]:>8,} / {seq[i-1]:>8,} = {seq[i]/seq[i-1]:.4f}")

# Check for linear recurrence of order k
print("\n--- Recurrence Hunting ---")

for order in range(2, 7):
    # Build system: a(n) = c1*a(n-1) + c2*a(n-2) + ... + ck*a(n-k)
    rows = len(seq) - order
    if rows < order:
        break
    A = np.zeros((rows, order))
    b = np.zeros(rows)
    for i in range(rows):
        for j in range(order):
            A[i, j] = seq[i + order - 1 - j]
        b[i] = seq[i + order]

    try:
        coeffs, residuals, _, _ = np.linalg.lstsq(A, b, rcond=None)
        # Verify
        max_err = 0
        for i in range(rows):
            pred = sum(coeffs[j] * seq[i + order - 1 - j] for j in range(order))
            err = abs(pred - seq[i + order])
            max_err = max(max_err, err)

        if max_err < 0.5:
            print(f"\n  *** EXACT recurrence of order {order} found! ***")
            terms = []
            for j in range(order):
                c = coeffs[j]
                if abs(c - round(c)) < 0.01:
                    c = int(round(c))
                terms.append(f"{c}*a(n-{j+1})")
            print(f"  a(n) = {' + '.join(terms)}")
            print(f"  Max error: {max_err:.6f}")
            break
        else:
            print(f"  Order {order}: max_err={max_err:.2f} (not exact)")
    except Exception as e:
        print(f"  Order {order}: {e}")

# Parity analysis
print("\n--- Parity Analysis (Even vs Odd N) ---")
even = [(i + 3, seq[i]) for i in range(len(seq)) if (i + 3) % 2 == 0]
odd = [(i + 3, seq[i]) for i in range(len(seq)) if (i + 3) % 2 == 1]
print(f"  Even N: {', '.join(f'{n}:{v}' for n, v in even)}")
print(f"  Odd  N: {', '.join(f'{n}:{v}' for n, v in odd)}")

# Asymptotic growth
if len(seq) >= 4:
    growth = seq[-1] / seq[-2]
    base = math.pow(seq[-1] / seq[0], 1.0 / (len(seq) - 1))
    print("\n--- Asymptotic ---")
    print(f"  Last ratio: {growth:.6f}")
    print(f"  Geometric mean base: {base:.6f}")
    print(f"  (Compare: golden ratio = {(1+math.sqrt(5))/2:.6f})")
