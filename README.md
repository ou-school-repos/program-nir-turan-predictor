# Computational Investigation of Leontovich Graphs

This repository contains source code, scripts, generated data, and a working
paper for computational experiments around graph homomorphism minimization from
trees to fixed target graphs.

The main mathematical focus is the Leontovich phenomenon: target graphs `H` for
which a non-path tree has fewer homomorphisms into `H` than the path on the same
number of vertices.

## Current Paper

The manuscript is in `paper/paper.tex`.

Build:

```bash
cd paper
pdflatex paper.tex
bibtex paper
pdflatex paper.tex
pdflatex paper.tex
```

The paper currently distinguishes between:

- exact algebraic arguments;
- exact integer computations for specific witnesses;
- exhaustive searches within explicitly bounded families;
- conditional floating-point filter sweeps followed by exact verification of
  flagged candidates.

Any claim that depends only on a floating-point filter should be treated as a
conditional sweep result, not an unconditional theorem.

## Key Verified Witnesses

- `T(7,1,9)` has a permanent `n=13` depth-2 crossover.
- `H*`, a 15-vertex bipartite graph with pattern `(0,1,6,4,1,0,0)`, is a
  depth-dependent bipartite Leontovich graph; the first verified hit is
  `(n,d)=(49,16)`.
- `H18`, pattern `(7,0,0,1,1,6,0)`, is an 18-vertex depth-2 bipartite
  Leontovich graph with
  `Hom(P_17,H18) - Hom(E_17^(2),H18) = 5,068,778`.
- The current 5-orbit strong-frontier audit distinguishes ordinary finite-window
  Leontovich behavior from strongly Leontovich behavior.

Run the fast exact witness check:

```bash
python3 scripts/verify_core_claims.py
```

## Repository Layout

- `paper/`: LaTeX manuscript and bibliography.
- `src/`: C++ search and verification kernels.
- `scripts/`: Python and C++ scripts for exact checks, plotting, search, and
  witness verification.
- `docs/`: generated figures, logs, and supporting notes.
- `proofs/`: Lean files and generated proof-related artifacts.

## Formal Verification Status

The Lean files in `proofs/` are not currently a complete formalization of the
paper. Some files are generated witnesses or exploratory proof artifacts, and
`proofs/SolverVerification.lean` is explicitly a placeholder.

At present, the most reliable verification layer for the headline computational
claims is exact integer arithmetic in Python/C++, not a Lean proof of the full
pipeline.

## Reproducibility Notes

The repository includes exploratory scripts and historical generated artifacts.
For review, prioritize:

```bash
python3 scripts/verify_core_claims.py
python3 scripts/verify_strong.py
cd paper && pdflatex paper.tex && bibtex paper && pdflatex paper.tex && pdflatex paper.tex
```

Longer exhaustive sweeps should be accompanied by the exact command line,
compiler version, machine information, and output hashes.
