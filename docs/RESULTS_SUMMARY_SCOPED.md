# Scoped Results Summary

This note restates the project's headline results with the same scope
discipline used in the manuscript and claims ledger.

## 1. Analytical Milestones

- **Path minimizer regimes:** For the odd paths `P3` and `P5`, the path
  minimizes the homomorphism count, but structural degeneracies create growing
  tie classes. Unique minimality begins at `P7`, where the squared dominant
  eigenvalue becomes irrational.
- **The `n=5` impossibility:** It is algebraically impossible for any
  spherically symmetric tree `T(x,y,z)` to cross over at `n=5`.
- **Spectral obstruction:** A rooted bipartite target whose active quotient
  matrix has exactly one positive eigenvalue cannot produce an odd-`n`
  near-path sign change. This analytically eliminates all 2-orbit and 3-orbit
  spherically symmetric trees in that setting.
- **`T(7,1,9)` threshold:** Among odd source orders, the Leontovich crossover
  threshold for `T(7,1,9)` is exactly `n=13`, and that threshold is permanent.

## 2. Key Computational Witnesses

- **`H*` (15 vertices):** This graph is a depth-dependent bipartite
  Leontovich graph. Its first verified hit is `(n,d) = (49,16)`.
- **`H18` (18 vertices):** Discovered via SMT-guided search and exact
  verification, this is the current depth-2 bipartite Leontovich witness found
  in the completed bounded sweep through path length `n <= 51`. Its first
  near-path crossover is `(n,d)=(15,4)`, with verified margin `500,576`; it
  crosses against `E_17^(2)` at `n=17` with margin `5,068,778`.
- **`\hat T(1,35,1,50)` (1,822 vertices):** This is the smallest strongly
  Leontovich looped symmetric tree in the audited 5-orbit frontier.

## 3. Disproving the Bipartite Parity Hypothesis

- The bipartite double-cover operation preserves the Leontovich property in
  the form used here.
- Applying the double cover to `\hat T(1,35,1,50)` yields a simple strongly
  Leontovich bipartite graph on `3,644` vertices.
- That `3,644`-vertex double-cover graph overtakes the path at the even
  threshold `n = 17,340`.
- This refutes the previously suggested universal bipartite parity hypothesis
  for even-`n` crossovers.

## 4. Bounds and Verification Methodology

- **Simple-target bounds:** An exhaustive sweep found no connected simple graph
  on `m <= 11` vertices producing a near-path violation in the tested range
  `n <= 200`, `d <= 20`.
- **Precision and verification:** Floating-point arithmetic was used only as a
  screening layer, with relative tolerance `1e-11`. The positive witnesses
  listed above were then checked with exact integer arithmetic.

## Scope Notes

- `H18` is not stated here as an unconditional globally minimal bipartite
  Leontovich graph; the bounded `n <= 51` depth-2 sweep is part of the claim.
- The `m <= 11` simple-target sweep is a bounded near-path nonexistence result,
  not a universal small-target theorem.
