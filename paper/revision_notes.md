# Revision Notes

## Introduction / Preliminaries

The main gap was not length but definition coverage. The new opening section now supplies the missing graph-homomorphism setup and the transfer-matrix identities the paper uses throughout:

- A walk-counting lemma and a spider-count lemma, which make the script formulas appear in two lines rather than by ad hoc expansion.
- The near-path family as a spider, `E_n^{(d)} = Sp(n-d-2, d, 1)`, so the symmetry `E_n^{(d)} \cong E_n^{(n-2-d)}` is visible rather than implicit.
- Equitable partitions, quotient reduction, and the weighted self-adjointness identity `s_i q_{ij} = s_j q_{ji}` by double edge-counting.
- The leading-coefficient ratio written as `ρ_d = ⟨w_1 ⊙ w_d, u⟩_s / (λ_1^{d+1} ⟨1, u⟩_s)`, matching the code’s `num/den` structure.
- A standards-of-evidence subsection with tags `(P)`, `(E)`, `(S)`, and `(N)`, so bounded sweeps stay distinct from proved statements.

## Vocabulary

Some words are technical and should stay:

- `spectral radius` is standard.
- `quotient eigenbasis expansion` is the right phrase where the proof is actually using that decomposition.

Some words should be avoided or narrowed:

- `spectral decomposition` is too generic unless it is immediately tied to the quotient-eigenbasis expansion.
- The section title should remain `An eigenvalue condition that forbids crossovers`; the older wording suggested the wrong direction.
- Repeated uses of `structural`, `spectral`, `mathematical`, `strictly`, and `landscape` should be trimmed where they are doing stylistic, not technical, work.

## Results and positioning

- The `T(7,1,9)` threshold result is not new in the informal sense; it was already discussed in correspondence. The paper should present it as the first complete written proof and place it in the broader threshold landscape.
- The `n = 5` theorem generalizes the earlier `T(x,y,z)` identity to all symmetric nonnegative targets.
- The `n = 6` residue is best stated as verified-plus-conjecture: three trees are proved, two are exact-computation checks, and the near-path sufficiency conjecture stays open.
- The double-cover transfer should be stated as a corollary, because it cleanly turns simple-graph sweep bounds into loopy-graph bounds.

## Specific places to revisit later

- Contribution (iv) still needs to stay aligned with the revised sweep language, not with the older unconditional lower bound.
- The statement that 76 vertices is the unique minimal size should be phrased carefully against the bounded-audit theorem.
- The same 1,801-vertex example is attributed in two places; that should be normalized to a single citation trail.

## Bottom line

The current draft is in the right shape:

- `T(7,1,9)` threshold: known in correspondence, proved here in full.
- New here: the 76-vertex witness, the simple-graph sweeps, and the cleaned-up evidence taxonomy.
- The revision work is mostly about phrasing, provenance, and evidence boundaries, not changing the theorem stack.
