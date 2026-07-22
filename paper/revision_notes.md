# Revision Notes

## Spectral audit

- Keep `spectral radius` as standard terminology.
- Replace generic `spectral decomposition` wording with `quotient eigenbasis expansion` where that is the actual mechanism.
- Keep the repaired theorem title `An eigenvalue condition that forbids crossovers`; the older phrasing suggested the opposite direction.
- The `P_5` tie phenomenon is explained by the active spectrum `{sqrt(3), 0, -sqrt(3)}` and the recurrence `w_{k+2} = 3 w_k`.
- The irrationality argument for `P_k` with `k >= 7` only rules out exact shift-symmetry; unique minimality for the verified range is computational, not a pure consequence of irrationality alone.

## Structural recommendations

- State the `n = 6` residue as verified-but-not-fully-proved.
- Present the double-cover transfer as a corollary of the main double-cover theorem, with the loopy-target bound stated explicitly.
- Keep the near-path filter results clearly labeled as bounded sweeps rather than nonexistence theorems.
- Use the `P_5` and `P_6` cases as the clean dividing line between proved algebraic identities and checked residue.

## Paper-level summary

- Section 3: the path-minimizer discussion is now organized around the exact tie classes at `P_3` and `P_5`, then the verified unique-minimality transition at `P_7`.
- Section 4: the main obstruction theorem is phrased as an eigenvalue condition that forbids crossovers.
- Section 5: the double-cover transfer is the bridge from simple sweeps to loopy targets, and the remaining small-tree residue is kept explicit.
