# A Computational-Analytic Hybrid Architecture for Extremal Network Logistics

**Author:** [Your Name]
**Theoretical Foundation:** The recent discrete extremal combinatorics research of Jonathan D. Nir et al. (2023–2026).

---

## Abstract

Standard differential equations and heuristic algorithms fail to capture the discrete topological bottlenecks inherent in complex, real-world networks. This project introduces a **Computational-Analytic Hybrid Architecture** that bridges the gap between chaotic, NP-hard state-space exploration and absolute formal verification.

By treating extremal graph theory not as abstract mathematics, but as the underlying physics of networks, this engine translates mathematical theorems into **Certified Physical Policies**.

1. **The C++ Solver (The Explorer):** Hardware-accelerated (AVX2/SIMD) engines utilize Bitboard POMDPs, Dynamic Programming, and greedy heuristics to navigate massive state spaces and find optimal sequences.
2. **The Lean 4 Verifier (The Judge):** The C++ engine generates an _Intervention Witness_ (a `.lean` file). Lean 4 compiles a deterministic Boolean checker, ingesting the C++ array, and uses the `native_decide` tactic to physically execute the simulation inside the compiler's trusted zero-trust kernel.

---

## Operational Modules & Theoretical Physics

### Urban Epidemiology & Containment Logistics

- **Math Basis:** _Achievable Burning Densities of Growing Grids (Gunderson, Nir, Pralat 2026)_
- **The Physics:** Simulating the spread of _Wolbachia_ vectors or wildfire retardant on rapidly expanding spatial grids.
- **The Engine:** A greedy C++ Look-Ahead heuristic operating on a pathological Comb Graph. It identifies topological bottlenecks that defy standard Burning Number Conjecture (BNC) limits. Lean 4 mechanically verifies the saturation sequence.

### Autonomous Drone Swarms & Cyber-Threat Hunting

- **Math Basis:** _The one-visibility localization game (Bonato, Marbach, Molnar, Nir 2024)_
- **The Physics:** Trapping an invisible, laterally moving target (APT malware or a physical evader) using strictly limited ($1$-visibility) telemetry.
- **The Engine:** A Bit-State POMDP solver utilizing Min-Max Entropy reduction. The engine iteratively drives the target's quantum belief state down to exactly 0, generating a mathematically certified, 0-blind-spot flight playbook.

### 6G Frequency Allocation & Signal Resilience

- **Math Basis:** _Hoffman-London graphs: When paths minimize q-colorings among trees (Galvin, McMillon, Nir 2026)_
- **The Physics:** Graph $q$-coloring maps directly to frequency channel allocation in IoT/6G networks.
- **The Engine:** A deep Dynamic Programming (DP) algorithm calculating exact valid 3-colorings. It computationally proves that specific "Leontovich" star-hub topologies are fundamentally more fragile and prone to signal contention than standard linear paths.

### Supply Chain & Financial Network Robustness

- **Math Basis:** _A localized approach to generalized Turán problems (Kirsch, Nir 2023)_
- **The Physics:** Systemic collapse occurs when a bipartite financial or supply-chain network exceeds its Turán limit, causing cyclic risk dependencies to non-linearly explode (Supersaturation).
- **The Engine:** A high-performance AVX2 SIMD engine that intersects 256-bit neighbor-masks to exactly count $K_3$ risk cycles in micro-seconds, flagging catastrophic breaches of Mantel's Theorem.

---

## Continuous Certification Pipeline

This repository is built as a unified `Makefile` pipeline. It does not merely log output; it continuously compiles and proves its own generated logistics.

```bash
# Build the unified C++ Hardware Solver
make build

# Execute and Certify the 4 Pipelines
make test/all
```

**Example Pipeline Execution (Surveillance POMDP):**

```text
[C++ Solver] Running POMDP Belief-State Reduction...
  [Turn  1] Drone Probes Node  1 -> Target Entropy Reduced To: 62 possible nodes.
  [Turn  2] Drone Probes Node  3 -> Target Entropy Reduced To: 60 possible nodes.
  ...
  [Turn 32] Drone Probes Node 62 -> Target Entropy Reduced To: 0 possible nodes.
  [Solver] Capture Guaranteed. Playbook Generated: proofs/ThreatHunting.lean

[OK] Verified: capture_guaranteed (native_decide evaluated to TRUE).
==================================================
CERTIFIED DRONE FLIGHT PLAYBOOK:
  [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 62, ]
MATHEMATICAL GUARANTEE: 0 blind spots. Evasion impossible.
==================================================
```

---

## Recent Discovery: Caterpillar Nash Equilibrium

We discovered a novel structural invariant in the adversarial Maker-Breaker burning game on caterpillar trees $C(S,K)$. Two game models yield two clean formulas, both independent of backbone length:

| Model                       | Fire Timing           | Nash Value ($S$ large) |
| --------------------------- | --------------------- | ---------------------- |
| Async (fast response)       | Between builder turns | $K + 2$                |
| Sync (standard firefighter) | Same turn as cut      | $2K + 2$               |

**Key insight**: Network vulnerability scales with hub degree, not backbone length.

See [docs/CATERPILLAR_NASH.md](docs/CATERPILLAR_NASH.md) for more details.

---

## Computer output (C++ and Lean 4)

\footnotesize

```log
Generating Epidemiology Policy [Scale: 64]
[Dendro] Initializing Spatial Comb Graph (N=64) for BNC Verification.
  -> Burning Number Conjecture Limit: b(G) <= 8 steps.
  [Step 1] Drop at Node 7 | Saturation: 1.6%
  [Step 2] Drop at Node 21 | Saturation: 7.8%
  [Step 3] Drop at Node 27 | Saturation: 20.3%
  [Step 4] Drop at Node 12 | Saturation: 39.1%
  [Step 5] Drop at Node 0 | Saturation: 62.5%
  [Step 6] Drop at Node 0 | Saturation: 82.8%
  [Step 7] Drop at Node 0 | Saturation: 95.3%
  [Step 8] Drop at Node 0 | Saturation: 100.0%
  [Success] Decreasing Radius Heuristic defeated local minima! Saturation in 8 steps.
Build completed successfully (3299 jobs).
[OK] Verified: policy_is_valid (native_decide evaluated to TRUE)
==================================================

CERTIFIED DEPLOYMENT LOGISTIC MAP:
  [7, 21, 27, 12, 0, 0, 0, 0, ]
theorem policy_satisfies_bnc : execute_burning grid_adj deployment_sequence = 0xFFFFFFFFFFFFFFFF /\ deployment_sequence.length <= 8 := by native_decide
MATHEMATICAL GUARANTEE: 100% network saturation achieved
==================================================

Generating Threat Hunting Playbook [Iter: 1000]
[Dendro] 1-Visibility POMDP Tracker on Binary Tree.
  [Success] Target isolated and captured in 33 deterministic probes.
Build completed successfully (3299 jobs).
[OK] Verified: capture_guaranteed (native_decide evaluated to TRUE)
==================================================

CERTIFIED DRONE FLIGHT PLAYBOOK:
  [15, 16, 3, 17, 18, 3, 19, 1, 9, 20, 1, 9, 1, 10, 0, 21, 22, 0, 23, 24, 0, 11, 2, 12, 6, 25, 26, 6, 27, 28, 6, 29, 30, ]
theorem capture_guaranteed : execute_hunt cave_adj drone_routing_playbook = 0 := by native_decide
MATHEMATICAL GUARANTEE: 0 blind spots. Evasion impossible
==================================================

Running Adversarial Burning
[Dendro] Adversarial: Grid 4x4 (16N, depth 8).
  -> Burner (Max) vs Builder (Min). Alpha-Beta with Zobrist TT.
  [Search] Alpha-Beta pruning (depth=8 plies)...
  [Ply 1] Burner drops on N13 (1 burned)
  [Ply 2] Builder severs edge 9-13 (1 burned)
  [Ply 3] Burner drops on N14 (2 burned)
  [Ply 4] Builder severs edge 10-14 (2 burned)
  [Ply 5] Burner drops on N12 (3 burned)
  [Ply 6] Builder severs edge 8-12 (3 burned)
  [Ply 7] Burner drops on N15 (4 burned)
  [Ply 8] Builder severs edge 11-15 (4 burned)
  [Nash Equilibrium] Builder limits destruction to 4/16 nodes.
  [Telemetry] 55196 states searched in 8 ms
[Dendro] Adversarial: Binary Tree (15) (15N, depth 12).
  -> Burner (Max) vs Builder (Min). Alpha-Beta with Zobrist TT.
  [Search] Alpha-Beta pruning (depth=12 plies)...
  [Ply 1] Burner drops on N0 (1 burned)
  [Ply 2] Builder severs edge 0-1 (1 burned)
  [Ply 3] Burner drops on N2 (2 burned)
  [Ply 4] Builder severs edge 0-2 (2 burned)
  [Ply 5] Burner drops on N6 (3 burned)
  [Ply 6] Builder severs edge 2-5 (3 burned)
  [Ply 7] Burner drops on N13 (4 burned)
  [Ply 8] Builder severs edge 6-14 (4 burned)
  [Ply 10] Builder severs edge 2-6 (4 burned)
  [Ply 12] Builder severs edge 6-13 (4 burned)
  [Nash Equilibrium] Builder limits destruction to 4/15 nodes.
  [Telemetry] 239145 states searched in 28 ms
[Dendro] Adversarial: Campus Network (16N, depth 14).
  -> Burner (Max) vs Builder (Min). Alpha-Beta with Zobrist TT.
  [Search] Alpha-Beta pruning (depth=14 plies)...
  [Ply 1] Burner drops on N3 (1 burned)
  [Ply 2] Builder severs edge 0-3 (1 burned)
  [Ply 3] Burner drops on N2 (2 burned)
  [Ply 4] Builder severs edge 0-2 (2 burned)
  [Ply 5] Burner drops on N8 (3 burned)
  [Ply 6] Builder severs edge 2-8 (3 burned)
  [Ply 7] Burner drops on N9 (4 burned)
  [Ply 8] Builder severs edge 2-9 (4 burned)
  [Ply 9] Burner drops on N10 (5 burned)
  [Ply 10] Builder severs edge 2-10 (5 burned)
  [Ply 11] Burner drops on N11 (6 burned)
  [Ply 12] Builder severs edge 2-11 (6 burned)
  [Ply 13] Burner drops on N12 (7 burned)
  [Ply 14] Builder severs edge 3-12 (7 burned)
  [Nash Equilibrium] Builder limits destruction to 7/16 nodes.
  [Telemetry] 2379372 states searched in 139 ms
Running Systemic Risk Audit [Scale: 64]
[Dendro] Turan Limits & Systemic Risk Centrality (N=64).
  -> Found 1032 edges (Mantel Limit = 1024). Supersaturation Triggered!
  -> Exact Triangles (K3): 258
[OK] All pipelines verified.
```

---

## Leontovich Tree Sweep (m=22)

Exhaustive sweep of all 5.6M non-isomorphic trees on 22 vertices,
testing each as a potential Leontovich graph target via the
`leontovich_fast` asymptotic spectral filter ($n \le 200$, $d \le 20$).

```shell
$ ./synthesizer 22 --export-g6 | ./leontovich_fast
[c++ synthesizer] Enumerating trees on N=22 (top_k=10)
  Path P(22) baseline: 46368 independent sets
  [c++] 5622K / 5623K (100%) | 6803.1s | 0K/s | ETA 2s | pruned 0.0M     [filter] 5620000 graphs | 0 hits | 826/s | 6798s elapsed
  ===============================================
  N=22 | 5623756 unique / 97055181 rooted | 6806.5s
  Throughput: 826 trees/sec | Dedup ratio: 17.3x
  Hash load: 33.5% (5623756 / 16777216 slots)
  Trees: 5623756 (d<=3: 254371, d<=4: 2278658)
  Top-1 (any): 2097153 (45.23x vs path)
  Top-1 (d<=3): 84546 | Top-1 (d<=4): 147177
  ===============================================
{
  "n": 22,
  "trees_scanned": 5623756,
  "rooted_processed": 97055181,
  "path_score": 46368,
  "elapsed_ms": 6806481.7,
  "trees_per_sec": 826,
  "top_k": [
    {
      "constraint": "Delta <= 3 (Chemical / 6G Routing)",
      "score": 84546,
      "ratio": 1.82,
      "max_degree": 3,
      "diameter": 6,
      "leaves": 12,
      "edges": [[0,1], [1,2], [2,3], [3,4], [4,5], [5,6], [5,7], [4,8], [8,9], [8,10], [3,11], [11,12], [12,13], [12,14], [11,15], [15,16], [15,17], [2,18], [18,19], [18,20], [1,21]]
    },
    {
      "constraint": "Delta <= 4 (Telecom Hubs)",
      "score": 147177,
      "ratio": 3.17,
      "max_degree": 4,
      "diameter": 5,
      "leaves": 15,
      "edges": [[0,1], [1,2], [2,3], [3,4], [4,5], [4,6], [4,7], [3,8], [8,9], [8,10], [8,11], [3,12], [12,13], [12,14], [12,15], [2,16], [16,17], [16,18], [16,19], [1,20], [1,21]]
    },
    {
      "constraint": "Unconstrained",
      "score": 2097153,
      "ratio": 45.23,
      "max_degree": 21,
      "diameter": 2,
      "leaves": 21,
      "edges": [[0,1], [1,2], [1,3], [1,4], [1,5], [1,6], [1,7], [1,8], [1,9], [1,10], [1,11], [1,12], [1,13], [1,14], [1,15], [1,16], [1,17], [1,18], [1,19], [1,20], [1,21]]
    }
  ]
}

Leontovich filter: 0 / 5623756 graphs flagged (n<=200, d<=20) | 6806s | 826 graphs/s
{"event":"leontovich_filter_done","total":5623756,"hits":0,"elapsed_s":6806,"rate_per_s":826}
```

---

## Bipartite Exhaustive Sweep ($m \leq 15$)

Exhaustive search of all connected bipartite graphs with up to 15 edges
(partitions 7+8), generated by `genbg` and filtered by `leontovich_fast`.

```shell
$ genbg -c 7 8 -q | ./leontovich_fast
  [filter] 457202935 graphs | 0 hits | 166910/s | 2739s elapsed
Leontovich filter: 0 / 457202935 graphs flagged (n<=200, d<=20) | 2739s | 166909 graphs/s
{"event":"leontovich_filter_done","total":457202935,"hits":0,"elapsed_s":2739,"rate_per_s":166909}
```

Combined with the all-connected sweep ($m \leq 10$): **no Leontovich graph exists with $m \leq 15$ edges** (bipartite) or **$m \leq 10$ edges** (general).

---

## 76-Vertex Leontovich Graph Discovery

Simulated annealing from T(7,1,9) discovered a 76-vertex Leontovich graph,
improving the previous minimum of 78 vertices. The graph is stored in
[`data/leontovich_76.json`](data/leontovich_76.json).

| Property              | T(7,1,9)    | 77-vertex   | 76-vertex   |
| --------------------- | ----------- | ----------- | ----------- |
| Vertices              | 78          | 77          | 76          |
| First crossover $n$   | 13          | 15          | 17          |
| $\lambda_1$           | 3.3973      | 3.3874      | 3.3766      |
| $\lambda_2$           | $\sqrt{10}$ | $\sqrt{10}$ | $\sqrt{10}$ |
| $\lambda_2/\lambda_1$ | 0.9308      | 0.9336      | 0.9365      |

**Current bounds:** smallest Leontovich graph has **$11 \leq m \leq 76$** edges
($16 \leq m \leq 76$ for bipartite graphs).

### Tools

```shell
# Verify the 76-vertex graph with exact integer arithmetic
python3 scripts/verify_76.py

# Plot the graph structure
python3 scripts/plot_76.py          # -> docs/out/leontovich_76.gif
python3 scripts/plot_76.py --png    # -> docs/out/leontovich_76.png

# C++ simulated annealing (1000x faster than Python)
make leontovich_sa
./leontovich_sa --steps 1000000 --seed 7 --start 7,1,9
./leontovich_sa --steps 1000000 --seed 42 --start L76
./leontovich_sa -h  # show all options
```

---

![Epidemiology: Comb Graph BNC Deployment](docs/epidemiology.svg)

![Autonomous Surveillance: POMDP Drone Playbook](docs/surveillance.svg)

![Financial Network: Supersaturation Risk Audit](docs/finance.svg)
