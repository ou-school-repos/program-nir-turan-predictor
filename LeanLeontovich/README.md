# LeanLeontovich

This directory is an experimental Lean project for the paper's definitions.
It is not a formal verification of the principal results.

It is intentionally separate from `legacy/`, which is used as the witness and
SMT-style verification layer.

Current axiom-free content:

- finite graph and homomorphism-count definitions
- path and near-path definitions
- a concrete bipartite double-cover definition
- the canonical lift of a homomorphism from a properly two-colored source

Named witnesses and their crossover properties remain in the `Assumed`
namespace. The obsolete local-SMT minimality and single-positive-eigenvalue
claims are intentionally absent. The next formalization target is the
two-to-one homomorphism-count identity for connected bipartite sources.

Build from this directory with:

```bash
lake build
```
