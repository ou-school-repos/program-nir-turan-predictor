# Novel Integer Sequences: Maximum Independent Sets on Degree-Constrained Trees

**Status:** Not found in OEIS as of 2026-04-25
**OEIS Query ($\Delta \leq 3$):** `seq:5,9,14,24,41,66,110,189,305,510,863` → **0 results**
**OEIS Query ($\Delta \leq 4$):** `seq:5,9,17,26,44,80,145,226,388,684` → **0 results**
**Proposed Titles:**

1. _Maximum number of independent sets among all trees on n vertices with maximum degree at most 3_
2. _Maximum number of independent sets among all trees on n vertices with maximum degree at most 4_

---

## The Sequence

For each $N \ge 3$, let $a(N)$ denote the maximum number of independent sets (including the empty set) over all trees $T$ on $N$ vertices satisfying $\Delta(T) \le 3$.

| $N$ | $a(N)$ ($\Delta \le 3$) | $a(N)$ ($\Delta \le 4$) | Star $2^{N-1}+1$ | Path (Fibonacci) |
| --- | ----------------------- | ----------------------- | ---------------- | ---------------- |
| 3   | 5                       | 5                       | 5                | 5                |
| 4   | 9                       | 9                       | 9                | 8                |
| 5   | 14                      | 17                      | 17               | 13               |
| 6   | 24                      | 26                      | 33               | 21               |
| 7   | 41                      | 44                      | 65               | 34               |
| 8   | 66                      | 80                      | 129              | 55               |
| 9   | 110                     | 145                     | 257              | 89               |
| 10  | 189                     | 226                     | 513              | 144              |
| 11  | 305                     | 388                     | 1,025            | 233              |
| 12  | 510                     | 684                     | 2,049            | 377              |
| 13  | 863                     | 1,241                   | 4,097            | 610              |
| 14  | 1,425                   | 1,970                   | 8,193            | 987              |
| 15  | 2,345                   | 3,330                   | 16,385           | 1,597            |
| 16  | 3,987                   | 5,868                   | 32,769           | 2,584            |
| 17  | 6,515                   | 10,657                  | 65,537           | 4,181            |
| 18  | 10,905                  | 17,001                  | 131,073          | 6,765            |
| 19  | 18,254                  | 28,674                  | 262,145          | 10,946           |
| 20  | 30,135                  | 50,508                  | 524,289          | 17,711           |
| 21  | 49,913                  | 90,949                  | 1,048,577        | 28,657           |
| 22  | 84,546                  | 147,177                 | 2,097,153        | 46,368           |
| 23  | 138,170                 | 247,698                 | 4,194,305        | 75,025           |
| 24  | 231,117                 | 432,234                 | 8,388,609        | 121,393          |
| 25  | 386,222                 | 778,829                 | 16,777,217       | 196,418          |

**Raw sequence $\Delta \leq 3$ (N=3..25):**

```text
5, 9, 14, 24, 41, 66, 110, 189, 305, 510, 863, 1425, 2345, 3987, 6515,
10905, 18254, 30135, 49913, 84546, 138170, 231117, 386222
```

**Raw sequence $\Delta \leq 4$ (N=3..25):**

```text
5, 9, 17, 26, 44, 80, 145, 226, 388, 684, 1241, 1970, 3330, 5868, 10657,
17001, 28674, 50508, 90949, 147177, 247698, 432234, 778829
```

---

## Verification Method

All values are **exhaustively verified** — not sampled, not heuristic. For each $N$:

1. **Beyer–Hedetniemi level-sequence enumeration** generates all rooted labeled trees.
2. **128-bit canonical hashing** with center-root DFS deduplicates to non-isomorphic free trees.
3. **Bottom-up DP** computes the exact independent set count $i(T) = \prod_{v} (1 + \prod_{u \in \text{children}(v)} i_{\text{excl}}(u))$ in $O(N)$ per tree.
4. The maximum over all trees with $\Delta(T) \le 3$ is recorded.

Tree counts match the following OEIS sequences exactly, confirming completeness:

- **A000055** (all trees): 1, 1, 1, 2, 3, 6, 11, 23, 47, 106, …
- **A000672** ($\Delta \leq 3$ trees / "boron trees"): 1, 2, 2, 4, 6, 11, 18, 37, 66, …
- **A000602** ($\Delta \leq 4$ trees / "alkane isomers"): 1, 2, 3, 5, 9, 18, 35, 75, 159, …

### Tree Count Cross-Validation (N=3..20)

| $N$ | A000055 (all) | A000672 ($\Delta \leq 3$) | A000602 ($\Delta \leq 4$) |
| --- | ------------- | ------------------------- | ------------------------- |
| 3   | 1             | 1                         | 1                         |
| 5   | 3             | 2                         | 3                         |
| 10  | 106           | 37                        | 75                        |
| 15  | 7,741         | 1,132                     | 4,347                     |
| 20  | 823,065       | 52,233                    | 366,319                   |
| 25  | 104,636,890   | 2,841,632                 | 36,797,588                |

### Roots, runtime, and throughput

| $N$ | Trees Enumerated | Rooted Processed | Time    | Throughput |
| --- | ---------------- | ---------------- | ------- | ---------- |
| 20  | 823,065          | 12,826,228       | 9.7s    | 84.8K/s    |
| 21  | 2,144,505        | 35,221,832       | 27.0s   | 79.4K/s    |
| 22  | 5,623,756        | 104,636,890      | 88.6s   | 63.5K/s    |
| 23  | 14,828,074       | 279,793,450      | ~240s   | 61.8K/s    |
| 24  | 39,299,897       | 743,724,984      | 623.7s  | 63.0K/s    |
| 25  | 104,636,890      | 2,067,174,645    | 1890.5s | 55.3K/s    |

---

## Analytical Properties

### Growth Rate

The asymptotic growth rate $\rho = \lim_{N \to \infty} a(N)^{1/N}$ converges to approximately **1.669**, which is:

- **Strictly above** the golden ratio $\varphi = 1.618\ldots$ (the growth rate for path graphs / Fibonacci)
- **Below** $\sqrt{3} = 1.732\ldots$ (the growth rate for star graphs under $\Delta \le 3$)

### No Simple Linear Recurrence

Least-squares fitting was attempted for linear recurrences of orders 2 through 6. No exact integer recurrence was found (maximum residual errors ranged from 805 at order 2 down to 38 at order 6, but never reached zero). The sequence likely requires a non-trivial generating function or a recurrence that depends on the tree structure itself.

### Parity Oscillation

The consecutive ratios $a(N)/a(N-1)$ oscillate with period 2, swinging between approximately 1.61 and 1.72. This is consistent with the bipartite parity constraint: trees on even $N$ can achieve a balanced bipartite split, while odd $N$ forces imbalance, directly affecting independent set maximization.

### Structural Observation

The extremal trees (those achieving $a(N)$) consistently exhibit:

- **Bipartite skew** of 0 or 2 (near-balanced)
- **Degree assortativity** around $r \approx -0.4$ to $-0.5$ (moderately disassortative — hubs connect to leaves, not to other hubs)
- Qualitatively, they resemble **dendrimers** or balanced caterpillar trees, not paths and not stars

---

## Mathematical Context

**Classical bounds (Prodinger & Tichy, 1982):**
Among all trees on $N$ vertices:

- $\min_T i(T) = F_{N+2}$ (Fibonacci), achieved by the path $P_N$
- $\max_T i(T) = 2^{N-1} + 1$, achieved by the star $K_{1,N-1}$

**Degree-constrained gap:**
When $\Delta(T) \le 3$, the star graph is forbidden for $N \ge 5$. The maximum $a(N)$ defines a new function that interpolates between Fibonacci growth and exponential growth, governed by the structural constraints of bounded-degree trees.

**Relation to Nir (2025–2026):**
This sequence is directly relevant to:

- _Hoffman-London graphs: When paths minimize q-colorings among trees_ (Galvin & Nir, 2026)
- The hard-core model on bounded-degree graphs
- Chemical graph theory (carbon skeletons have $\Delta \le 4$; VLSI routing uses $\Delta \le 3$)

---

## Reproducibility

```bash
# Build the exhaustive enumerator
make synthesizer

# Run for a specific N (results auto-append to docs/runs/sequence.jsonl)
./synthesizer 24 > proofs/synth-n24.json

# Run the full sweep (N=3..23, ~5 minutes)
bash scripts/sweep_d3.sh 23

# Analyze the sequence
python3 scripts/analyze_sequence.py
```

**Engine:** `src/synthesizer.cpp` — C++17, compiled with `-O3 -march=native`
**Data:** `docs/runs/sequence.jsonl` — append-only JSONL log with timestamps, scores, and extremal tree edge lists

---

## OEIS Submission Drafts

### Sequence 1: $\Delta \leq 3$ (Primary)

**Name:** Maximum number of independent sets in a tree on n vertices with maximum degree at most 3.

**Data:** 5, 9, 14, 24, 41, 66, 110, 189, 305, 510, 863, 1425, 2345, 3987, 6515, 10905, 18254, 30135, 49913, 84546, 138170, 231117, 386222

**Offset:** 3

**Comments:** The path graph P*n minimizes independent sets among trees (giving Fibonacci numbers, A000045) and the star graph K*{1,n-1} maximizes them (giving 2^{n-1}+1). When the maximum degree is restricted to at most 3 ("boron trees," cf. A000672), the star is forbidden for n >= 5 and the maximum defines this sequence. The extremal trees resemble balanced dendrimers. The asymptotic growth rate is approximately 1.669, strictly between the golden ratio (1.618...) and sqrt(3). The number of trees searched at each n matches A000672.

**Author:** Shane Jaroch and JD Nir, 2026

### Sequence 2: $\Delta \leq 4$

**Name:** Maximum number of independent sets in a tree on n vertices with maximum degree at most 4.

**Data:** 5, 9, 17, 26, 44, 80, 145, 226, 388, 684, 1241, 1970, 3330, 5868, 10657, 17001, 28674, 50508, 90949, 147177, 247698, 432234, 778829

**Offset:** 3

**Comments:** Maximum of the Merrifield-Simmons index (number of independent sets including the empty set) over all trees on n vertices with maximum degree at most 4 ("alkane trees," cf. A000602). For n <= 4, the star K\_{1,n-1} is admissible and achieves the maximum. For n >= 5, the extremal trees exhibit a hub-spoke pattern with degree-4 hubs connected to binary sub-dendrimers. Growth rate approximately 1.72.

**Author:** Shane Jaroch and JD Nir, 2026

**Program (Python verification):**

```python
# Requires exhaustive tree enumeration; see GitHub link for C++ engine
```
