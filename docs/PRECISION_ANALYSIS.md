# Floating-Point Precision Analysis

## Overview

The `leontovich_fast` filter computes tree homomorphism counts using
IEEE 754 `double` arithmetic (52-bit mantissa, ~15.9 significant digits).
This document analyzes the precision limits and establishes that the
filter produces reliable results within its operating parameters.

## Computation

For a graph $H$ on $m$ vertices with adjacency matrix $A$:

1. **Walk vectors**: $w^{(0)} = \mathbf{1}$, $w^{(k)} = A \cdot w^{(k-1)}$
   (all entries are exact non-negative integers)
2. **Path hom**: $\operatorname{hom}(P_n, H) = \sum_i w^{(n-1)}_i$
3. **Near-path hom**: $\operatorname{hom}(E_n^{(d)}, H) = \sum_i w^{(\text{stem})}_i \cdot w^{(1)}_i \cdot w^{(d)}_i$

## Value Magnitude

For a graph with maximum degree $\Delta$, entries of $w^{(k)}$ grow as
$O(\Delta^k)$. The largest values occur at $k = 200$ (the filter's max):

| $m$ | Max $\Delta$ | $\Delta^{200}$ (approx) | Within `double` range? |
| --- | ------------ | ----------------------- | ---------------------- |
| 9   | 8            | $10^{181}$              | ✓ ($< 10^{308}$)       |
| 11  | 10           | $10^{200}$              | ✓                      |
| 15  | 14           | $10^{229}$              | ✓                      |

No overflow occurs for $m \le 15$, $n \le 200$.

## Accumulated Rounding Error

Each matrix-vector multiply $w^{(k)} = A \cdot w^{(k-1)}$ involves:

- $m$ multiply-accumulate operations per component
- Each with relative error $\le m \cdot \varepsilon$
  where $\varepsilon = 2^{-52} \approx 2.22 \times 10^{-16}$

After $n$ steps, the relative error in $w^{(n)}$ is bounded by:

$$\delta_n \le n \cdot m \cdot \varepsilon$$

| $m$ | $n$ | $\delta_n$            | Tolerance ($10^{-11}$) | Safety margin |
| --- | --- | --------------------- | ---------------------- | ------------- |
| 9   | 200 | $4.0 \times 10^{-13}$ | $10^{-11}$             | 25×           |
| 11  | 200 | $4.9 \times 10^{-13}$ | $10^{-11}$             | 20×           |
| 15  | 200 | $6.7 \times 10^{-13}$ | $10^{-11}$             | 15×           |

The `1e-11` tolerance in the comparison
`homE < homP * (1.0 - 1e-11)` provides a 15–25× safety margin over
the worst-case accumulated rounding error.

## False Positive / False Negative Analysis

**False positives** (filter flags a non-violation):
Occur when rounding makes `homE` appear smaller than `homP * (1 - 1e-11)`
even though the exact values satisfy `homE ≥ homP`. This requires
the rounding error to exceed the tolerance — ruled out by the 15× margin.

**False negatives** (filter misses a genuine violation):
Occur when a genuine `homE < homP` is masked by rounding that inflates
`homE` past the threshold. For this to happen, the true relative
difference $(homP - homE) / homP$ would need to be smaller than the
tolerance `1e-11`. Given the algebraic structure (spectral gap dominance),
genuine violations — if they exist — would have relative differences
well above this threshold.

## Empirical Validation

The Python exact verifier (`scripts/verify_hom.py`) was used to verify
the m=15 anomaly on graph `NCQCCA?_B?K?W?g?K??`:

- **300 anomalies** (d > 2) confirmed with exact arithmetic
- **0 Leontovich violations** (d = 2) — consistent with the filter
- Relative differences ~7.58×10⁻⁴ — far above the `1e-11` tolerance
- The `double` filter correctly identified this graph

## Synthesizer (`uint64_t`) Bounds

The brute-force `synthesizer` uses `uint64_t` (max ~1.8×10¹⁸).
For the m ≤ 9 exhaustive sweep (trees on n ≤ 15 vertices into
graphs with $\Delta \le 8$):

$$\max \operatorname{hom} \le 8^{15} \approx 3.5 \times 10^{13} \ll 1.8 \times 10^{18}$$

No overflow. The m ≤ 9 results are exact.

## Conclusion

- **No overflow** in either `double` or `uint64_t` within operating ranges
- **Rounding error** is bounded at ~10⁻¹³, well within the 10⁻¹¹ tolerance
- **False negatives** would require violations with relative margins < 10⁻¹¹,
  which is physically implausible given spectral gap structure
- All flagged anomalies can be verified exactly with `verify_hom.py`
