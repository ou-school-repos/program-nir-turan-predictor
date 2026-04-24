# Program Nir-Turan Predictor

A Computational-Analytic Hybrid Architecture for resolving open problems in discrete extremal combinatorics and vector ecology.

## Overview
This project combines hardware-accelerated C++ "Oracles" with Lean 4 formal verification "Squeezers" to bridge the gap between discrete branching complexity (NP-hard) and continuous algebraic bounds.

### Research Phases
1. **Graph Burning Conjecture (BNC):** Hardware-abusing AVX-512 BFS engines verifying $b(G) \le \lceil \sqrt{n} \rceil$ via continuous polyhedral relaxations.
2. **Localized Generalized Turán Problems:** Verifying $ex(n, T, F)$ saturation thresholds using SWAR neighborhood intersections and localized weight functions.
3. **Pursuit-Evasion (Localization Game):** Establishing bounding limits for $\zeta_k(G)$ using Belief-State MCTS and topological end-space formalization.
4. **Combinatorial Hopf Algebras (WMat):** Accelerating polynomial invariant generation (Tutte, Billera-Jia-Reiner) over complex topological structures using bit-vector matroids.

## Usage
### C++ Oracles
Build all oracles using the minimal Makefile:
```bash
make all
```

### Lean 4 Verifiers
Verify the algebraic proofs:
```bash
make lean
```

### Development
Format source code:
```bash
make format
```
