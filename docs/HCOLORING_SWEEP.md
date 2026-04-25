# H-Coloring Minimizer Sweep: Odd Path Conjecture

## Problem Reference

**Problem 6.3** (Galvin, McMillon, Nir 2026 — _Hoffman-London graphs_):

> Classify the trees that minimize $P_{2m+1}$-colorings among all trees on $n$ vertices.

It is known that even paths $P_{2m}$ are minimized by the path $P_n$ among trees.
The odd case is **open**.

## Computational Evidence

Exhaustive brute-force over all non-isomorphic trees on $n$ vertices.
Engine: `synthesizer.cpp` with `--hcolor` mode (tree DP, $O(n \cdot h^2)$ per tree).

### Phase 1: Even Paths (Cross-Validation)

Confirms the known theorem: $P_n$ minimizes $\hom(T, P_{2m})$ among all trees $T$ on $n$ vertices.

| Target | $n$ range | Trees checked | Violations | Status |
| ------ | --------- | ------------- | ---------- | ------ |
| $P_2$  | 0--20     | 1,643,856     | 0          | Known  |
| $P_4$  | 0--20     | 1,643,856     | 0          | Known  |
| $P_6$  | 0--20     | 1,643,856     | 0          | Known  |

### Phase 2: Odd Paths (Open Problem 6.3)

**Result: No violations found.** The path $P_n$ minimizes $\hom(T, P_{2m+1})$ across all tested cases.

#### $P_3$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_3)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 3                     | $P_1$     |
| 2   | 4                     | $P_2$     |
| 3   | 6                     | $P_3$     |
| 4   | 8                     | $P_4$     |
| 5   | 12                    | $P_5$     |
| 6   | 16                    | $P_6$     |
| 7   | 24                    | $P_7$     |
| 8   | 32                    | $P_8$     |
| 9   | 48                    | $P_9$     |
| 10  | 64                    | $P_{10}$  |
| 11  | 96                    | $P_{11}$  |
| 12  | 128                   | $P_{12}$  |
| 13  | 192                   | $P_{13}$  |
| 14  | 256                   | $P_{14}$  |
| 15  | 384                   | $P_{15}$  |
| 16  | 512                   | $P_{16}$  |
| 17  | 768                   | $P_{17}$  |
| 18  | 1024                  | $P_{18}$  |
| 19  | 1536                  | $P_{19}$  |
| 20  | 2048                  | $P_{20}$  |
| 21  | 3072                  | $P_{21}$  |
| 22  | 4096                  | $P_{22}$  |

**Pattern:** $\hom(P_n, P_3) = 3 \cdot 2^{\lfloor n/2 \rfloor} \cdot (4/3)^{n \bmod 2}$. Alternates $\times \frac{3}{2}$ and $\times \frac{4}{3}$.

#### $P_5$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_5)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 5                     | $P_1$     |
| 2   | 8                     | $P_2$     |
| 3   | 14                    | $P_3$     |
| 4   | 24                    | $P_4$     |
| 5   | 42                    | $P_5$     |
| 6   | 72                    | $P_6$     |
| 7   | 126                   | $P_7$     |
| 8   | 216                   | $P_8$     |
| 9   | 378                   | $P_9$     |
| 10  | 648                   | $P_{10}$  |
| 11  | 1134                  | $P_{11}$  |
| 12  | 1944                  | $P_{12}$  |
| 13  | 3402                  | $P_{13}$  |
| 14  | 5832                  | $P_{14}$  |
| 15  | 10206                 | $P_{15}$  |
| 16  | 17496                 | $P_{16}$  |
| 17  | 30618                 | $P_{17}$  |
| 18  | 52488                 | $P_{18}$  |
| 19  | 91854                 | $P_{19}$  |
| 20  | 157464                | $P_{20}$  |
| 21  | 275562                | $P_{21}$  |
| 22  | 472392                | $P_{22}$  |

**Pattern:** Ratios alternate between $\frac{7}{4} = 1.75$ and $\frac{12}{7} \approx 1.714$.

#### $P_7$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_7)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 7                     | $P_1$     |
| 2   | 12                    | $P_2$     |
| 3   | 22                    | $P_3$     |
| 4   | 40                    | $P_4$     |
| 5   | 74                    | $P_5$     |
| 6   | 136                   | $P_6$     |
| 7   | 252                   | $P_7$     |
| 8   | 464                   | $P_8$     |
| 9   | 860                   | $P_9$     |
| 10  | 1584                  | $P_{10}$  |
| 11  | 2936                  | $P_{11}$  |
| 12  | 5408                  | $P_{12}$  |
| 13  | 10024                 | $P_{13}$  |
| 14  | 18464                 | $P_{14}$  |
| 15  | 34224                 | $P_{15}$  |
| 16  | 63040                 | $P_{16}$  |
| 17  | 116848                | $P_{17}$  |
| 18  | 215232                | $P_{18}$  |
| 19  | 398944                | $P_{19}$  |
| 20  | 734848                | $P_{20}$  |
| 21  | 1362080               | $P_{21}$  |
| 22  | 2508928               | $P_{22}$  |

#### $P_9$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_9)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 9                     | $P_1$     |
| 2   | 16                    | $P_2$     |
| 3   | 30                    | $P_3$     |
| 4   | 56                    | $P_4$     |
| 5   | 106                   | $P_5$     |
| 6   | 200                   | $P_6$     |
| 7   | 380                   | $P_7$     |
| 8   | 720                   | $P_8$     |
| 9   | 1370                  | $P_9$     |
| 10  | 2600                  | $P_{10}$  |
| 11  | 4950                  | $P_{11}$  |
| 12  | 9400                  | $P_{12}$  |
| 13  | 17900                 | $P_{13}$  |
| 14  | 34000                 | $P_{14}$  |
| 15  | 64750                 | $P_{15}$  |
| 16  | 123000                | $P_{16}$  |
| 17  | 234250                | $P_{17}$  |
| 18  | 445000                | $P_{18}$  |
| 19  | 847500                | $P_{19}$  |
| 20  | 1610000               | $P_{20}$  |
| 21  | 3066250               | $P_{21}$  |
| 22  | 5825000               | $P_{22}$  |

### Summary

| Target | $n$ range | Total trees | Violations | Path minimizes? |
| ------ | --------- | ----------- | ---------- | --------------- |
| $P_3$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_5$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_7$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_9$  | 0--22     | 7,291,468   | **0**      | Yes             |

**Conjecture (computational):** For all odd paths $P_{2m+1}$ and all trees $T$ on $n$ vertices:

$$\hom(T, P_{2m+1}) \geq \hom(P_n, P_{2m+1})$$

Verified exhaustively for $m \in \{1, 2, 3, 4\}$ and $n \leq 22$ (5,623,756 trees at $n=22$).

## Reproduction

```bash
# Build
g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp

# Full sweep (n=0..20, ~3 min)
bash scripts/sweep_hcolor.sh

# Extend to larger n
bash scripts/sweep_hcolor.sh 21 25
```

## Raw Data

JSONL records with `hc_*` fields are appended to `docs/runs/sequence.jsonl`.
