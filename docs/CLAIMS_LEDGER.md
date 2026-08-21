# Core Claims Ledger

This file is the compact reference sheet for the paper's headline claims.
It reduces drift between manuscript prose, exact verifiers, and review
artifacts by keeping the main constants, bounds, evidence tags, and
reproducer commands in one place.

Evidence tags follow
[paper/sections/00_introduction.tex](../paper/sections/00_introduction.tex#L80):

- `P`: analytic proof
- `E`: exact finite computation
- `S`: bounded computational sweep
- `N`: conditional floating-point screen

## Claim Table

| Id                        | Claim                                                                           | Evidence               | Scope / bounds                                                 | Authoritative value(s)                                                | Reproducer                              |
| ------------------------- | ------------------------------------------------------------------------------- | ---------------------- | -------------------------------------------------------------- | --------------------------------------------------------------------- | --------------------------------------- |
| `n5-impossible`           | No `T(x,y,z)` near-path crossover at `n=5`                                      | `P`                    | Family `T(x,y,z)`                                              | Proposition `prop:n5`                                                 | `make verify/core`                      |
| `n6-residue`              | Four of six source-tree comparisons at `n=6` are proved; two remain open        | `P` + bounded evidence | Targets `T(x,y,z)`; source trees on 6 vertices                 | Two unresolved cases reduce to explicit cubic inequalities            | No universal reproducer                 |
| `t719-threshold`          | `T(7,1,9)` has permanent odd depth-2 threshold `n=13`                           | `P` + `E`              | Named 4-orbit tree; odd source orders                          | `Δ_11=-7434`, `Δ_13=48258`                                            | `make verify/core`                      |
| `hstar-witness`           | `H*` is the 15-vertex depth-dependent bipartite witness                         | `E`                    | Named graph only                                               | 300 hits, first `(n,d)=(49,16)`, no depth-2 hits                      | `python3 scripts/verify_core_claims.py` |
| `h18-margin`              | `H18` first crosses at `(n,d)=(15,4)` and has a depth-2 crossover at `n=17`     | `E`                    | Named graph; every admissible depth checked below `n=15`       | Margin `500,576` at `(15,4)`; depth-2 margin `5,068,778` at `n=17`    | `python3 scripts/verify_core_claims.py` |
| `h18-scope`               | `H18` is the current 18-vertex depth-2 bipartite witness in the completed sweep | `S`                    | Bipartite partition sweeps through path length `n<=51`         | No smaller hit in tested depth-2 regime                               | `make verify/core`                      |
| `small-simple-sweep`      | No connected simple graph on `m<=11` produced a near-path hit                   | `S`                    | `n<=200`, `d<=20`; all connected simple graphs on `m<=11`      | `1,018,690,329` tested, zero hits                                     | See `paper.tex` sweep table             |
| `bipartite-parity-screen` | Exhaustive bipartite screen found no even-`n` crossover below 16 vertices       | `S`                    | Connected bipartite graphs on `m<=15`, source trees on `n<=22` | `608,930,559` targets, zero even-`n` hits                             | Appendix discussion in `paper.tex`      |
| `strong-1822`             | `\hat T(1,35,1,50)` is strongly Leontovich                                      | `E`                    | Named 5-orbit looped symmetric tree                            | `rho=0.999953714414`, high-`n` ratio at `n=15001` is `0.999864892078` | `python3 scripts/verify_core_claims.py` |
| `even-17340`              | The bipartite double cover has first even crossover at `n=17,340`               | `E`                    | Double cover of `\hat T(1,35,1,50)`                            | negative at `17338`, positive at `17340`                              | `python3 scripts/verify_core_claims.py` |
| `table8-audit`            | Table 8 open/close windows were audited exactly                                 | `E`                    | Named table entries only                                       | finite-window open/close thresholds printed by verifier               | `python3 scripts/verify_strong.py`      |

## Notes

- If a statement is bounded by `n<=51`, `n<=200`, `d<=20`, or a vertex cap,
  that bound belongs in the sentence stating the claim.
- Anything discovered first by a floating-point search remains `N` until an
  exact verifier promotes the specific witness or threshold to `E`.
- This file is intentionally conservative. If a paper sentence is stronger
  than the matching row here, the sentence should be treated as suspect.
