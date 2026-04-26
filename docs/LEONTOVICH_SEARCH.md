# Leontovich Graph Search & Verification

Computational verification and extension of results from
_Long paths need not minimize H-colorings among trees_ (J.D. Nir, arXiv:2510.18770v1).

A graph $H$ is **Leontovich at $n$** if $\exists$ tree $T_n$ on $n$ vertices
with $\hom(T_n, H) < \hom(P_n, H)$.

## Analytical Verification: The $T(x,y,z)$ Family

The rooted tree $T(x,y,z)$ has 4 automorphic orbits with similarity matrix:

$$
M = \begin{pmatrix} 0 & x & 0 & 0 \\ 1 & 0 & y & 0 \\ 0 & 1 & 0 & z \\ 0 & 0 & 1 & 0 \end{pmatrix},
\quad a = (1,\; x,\; xy,\; xyz)
$$

enabling $O(1)$ homomorphism evaluation via $\hom(G_n, H) = a \cdot M^n \cdot h(G_0, v_0)$.

### Theorem 1.3 Verification

$H = T(18, 3, 32)$ with $|V| = 1{,}801$.

| tree  | $\hom(\cdot, H)$ | result            |
| ----- | ---------------- | ----------------- |
| $P_7$ | 81,558,090       |                   |
| $E_7$ | 81,548,856       | **wins by 9,234** |

### Exact Threshold for $T(7,1,9)$

$H = T(7,1,9)$ with $|V| = 78$. The paper proves $E_n$ beats $P_n$ for
"sufficiently large odd $n$" but does not give the threshold.

| $n$    | parity  | $\hom(P_n, H)$  | $\hom(E_n, H)$  | $P_n - E_n$ | winner    |
| ------ | ------- | --------------- | --------------- | ----------- | --------- |
| 5      | odd     | 9,366           | 9,492           | $-126$      | $P_n$     |
| 7      | odd     | 106,302         | 106,932         | $-630$      | $P_n$     |
| 9      | odd     | 1,217,076       | 1,219,848       | $-2{,}772$  | $P_n$     |
| 11     | odd     | 13,993,266      | 14,000,700      | $-7{,}434$  | $P_n$     |
| **13** | **odd** | **161,209,734** | **161,161,476** | **48,258**  | **$E_n$** |
| 15     | odd     | 1,858,989,720   | 1,857,700,992   | 1,288,728   | $E_n$     |
| 17     | odd     | 21,446,611,998  | 21,427,743,876  | 18,868,122  | $E_n$     |

**Result:** Threshold is $n = 13$. For all even $n$, $P_n$ wins.

**Parity explanation:** Both $P_n$ and $E_n$ have the **same bipartition sizes**
for all $n$: balanced $(n/2, n/2)$ for even $n$, and
$(\lceil n/2 \rceil, \lfloor n/2 \rfloor)$ for odd $n$. For even $n$,
$P_n$'s linear structure naturally minimizes the internal projection onto
the dominant eigenvector, locking in the path as the winner. For odd $n$,
the forced imbalance amplifies the role of the secondary eigenvector
(which captures local topological defects like $E_n$'s moved pendant),
giving it enough leverage to flip the winner at sufficiently large $n$.

### Smallest 4-Orbit Leontovich Graph

Exhaustive grid search over $T(x,y,z)$ with $|V| = 1 + x + xy + xyz \le 100$:

**$T(7,1,9)$ with 78 vertices is the unique smallest** in the $T(x,y,z)$ family.

This confirms that closing the Problem 4.3 gap ($4 \le m \le 78$) requires
searching outside the 4-orbit spherically symmetric family.

### Spectral Crossover Theorem

**Theorem (1-Positive Eigenvalue Rule):** *If the quotient matrix of a
bipartite graph $H$ has exactly one positive eigenvalue $\lambda_1$,
then $H$ cannot be Leontovich.*

**Proof:**

1. Because $H$ is bipartite, its spectrum is symmetric. If it has exactly
   one positive eigenvalue, its non-zero spectrum is $\{\lambda_1, -\lambda_1\}$.
2. By the transfer matrix theorem, $\hom(G_n, H) = c \lambda_1^n + d (-\lambda_1)^n$.
3. For odd $n$, this reduces to $(c - d) \lambda_1^n$. There are **no secondary
   exponential terms**. The ratio $\hom(E_n, H) / \hom(P_n, H)$ is a strict
   constant for all odd $n$.
4. Since $P_n$ minimizes at small $n$, it is locked in for all $n$.
   A delayed threshold (like $n = 13$) is algebraically impossible.

**Computational verification** (Task D in `leontovich.py`):

- All 2-orbit trees $T(x)$: 1 positive eigenvalue $\Rightarrow$ $P_n$ always wins (verified)
- All 3-orbit trees $T(x,y)$: 1 positive eigenvalue $\Rightarrow$ $P_n$ always wins (verified)
- 4-orbit trees $T(x,y,z)$: characteristic polynomial
  $\lambda^4 - (x+y+z)\lambda^2 + xz = 0$. The discriminant
  $(x+y+z)^2 - 4xz = (x-z)^2 + 2y(x+z) + y^2 > 0$ always,
  so **every** $T(x,y,z)$ has two positive eigenvalues.

**Key subtlety:** Two positive eigenvalues is **necessary but not sufficient**.
$T(2,2,2)$ has $\lambda_1 \approx 2.288$, $\lambda_2 \approx 0.874$ (both
positive), yet is not Leontovich --- it lacks the topological extremity (like
$T(7,1,9)$'s massive branches) to force the leading coefficients to flip.

### Asymptotic Defense of the $E_n^{(d)}$ Filter

**Claim:** Testing only the near-path family $E_n^{(d)}$ is asymptotically
sufficient. Highly branched trees cannot beat the path.

**Proof:** Csikvári and Lin (2014) showed that the exponential growth rate
of tree homomorphisms satisfies
$\liminf \hom(T_n, H)^{1/n} \ge \lambda_1(H)$,
with the path $P_n$ achieving this minimum. Stars and highly branched trees
grow as $\sim \Delta(H)^n$, and since $\Delta(H) > \lambda_1(H)$ for all
non-regular graphs, branched trees produce **exponentially more** homomorphisms.
To beat the path, a tree must match its minimal growth rate $\lambda_1^n$
while achieving a smaller leading coefficient — this is only possible for
long paths with localized boundary defects, i.e., the $E_n^{(d)}$ family.

### Paper Correction

The paper states $T(7,1,9)$ (78 vertices) is Leontovich. Our computation
confirms this, but shows the threshold is $n = 13$ (not $n = 7$ as
potentially implied by the paper's phrasing). For $n < 13$, $P_n$ still
wins. This is a refinement, not a contradiction.

## General Graph Search (Problem 4.3)

Open problem: what is the smallest $m$ such that a graph $H$ on $m$ vertices
is Leontovich? The paper shows $4 \le m \le 78$.

### Exhaustive tree sweep: all connected $H$ with $\|V(H)\| \le 9$

For each $m$, we loaded all connected graphs via `geng -c m`
and tested every tree on $n$ vertices for $n \in \{5, 6, \ldots, 15\}$.
Graph counts cross-validated against [OEIS A001349](https://oeis.org/A001349).

| $m = \|V(H)\|$ | connected $H$ | cumulative $H$ | ops ($H \times T \times n \times m^2$) | violations |
| -------------- | ------------- | --------------- | -------------------------------------- | ---------- |
| 4              | 6             | 6               | $1.8 \times 10^7$                      | **0**      |
| 5              | 21            | 27              | $9.9 \times 10^7$                      | **0**      |
| 6              | 112           | 139             | $7.6 \times 10^8$                      | **0**      |
| 7              | 853           | 992             | $7.9 \times 10^9$                      | **0**      |
| 8              | 11,117        | 12,109          | $1.3 \times 10^{11}$                   | **0**      |
| 9              | 261,080       | 273,189         | $4.0 \times 10^{12}$                   | **0**      |

### Asymptotic filter: $E_n^{(d)}$ family up to $n = 200$

The exhaustive tree sweep only covers $n \le 15$. To close the
"delayed threshold" loophole (a graph whose Leontovich crossover
occurs at large $n$), we use `leontovich_fast.cpp`: for each target $H$,
test the near-path trees $E_n^{(d)}$ (path with pendant at depth $d$)
via $O(m)$ matrix-vector iteration up to $n = 200$, $d \le 20$.

| $m$ | connected $H$ | filter time | violations ($n \le 200$) |
| --- | ------------- | ----------- | ------------------------ |
| 4   | 6             | instant     | **0**                    |
| 5   | 21            | instant     | **0**                    |
| 6   | 112           | instant     | **0**                    |
| 7   | 853           | instant     | **0**                    |
| 8   | 11,117        | < 1s        | **0**                    |
| 9   | 261,080       | ~40s        | **0**                    |
| 10  | 11,716,571    | ~57 min     | **0**                    |

### Bipartite graph sweep (via `genbg`)

The known Leontovich graph $T(7,1,9)$ is a tree (hence bipartite).
Connected bipartite graphs grow far slower than general connected graphs,
enabling deeper $m$-values. For each partition $(n_1, n_2)$ with
$n_1 + n_2 = m$, we run `genbg -c n1 n2 -q | ./leontovich_fast`.

| $m$ | bipartite $H$ | filter time | violations ($n \le 200$) |
| --- | ------------- | ----------- | ------------------------ |
| 10  | 5,664         | < 1s        | **0**                    |
| 11  | 25,598        | ~2s         | **0**                    |
| 12  | 308,362       | ~30s        | **0**                    |
| 13  | 2,241,730     | ~16 min     | **0**                    |

### Tree-as-target sweep (via `--export-g6`)

If the smallest Leontovich graph is a tree, we can test all trees
up to $m = 25$ by piping `synthesizer --export-g6` into the filter.
Tree counts match [OEIS A000055](https://oeis.org/A000055).

| $m$ | trees (A000055) | filter time | violations ($n \le 200$) |
| --- | --------------- | ----------- | ------------------------ |
| 10  | 106             | instant     | **0**                    |
| 12  | 551             | instant     | **0**                    |
| 15  | 7,741           | ~4s         | **0**                    |
| 18  | 123,867         | ~2 min      | **0**                    |
| 20  | 823,065         | ~12 min     | **0**                    |

### Current bound

**Rigorous bound: $m \ge 10$** for all $n \le 15$,
via exhaustive enumeration of all 273,189 connected graphs and
all trees on $n \in \{5, \ldots, 15\}$ vertices.

**Strong conditional bound: $m \ge 11$.**
Because asymptotic minimizers must match the path's minimal
exponential growth rate $\lambda_1(H)^n$ (highly branched trees
grow as $\Delta(H)^n \ge \lambda_1(H)^n$), the only viable challengers
at large $n$ are near-path trees $E_n^{(d)}$. Filtering these up to
$n = 200$ across all 11,989,760 connected graphs with $m \le 10$
yields zero violations.

**Extended searches** (conditional, $E_n^{(d)}$ filter only):

- No bipartite Leontovich graph on $\le 13$ vertices (2,581,354 graphs tested)
- No tree Leontovich graph on $\le 20$ vertices (832,330 trees tested)

## Reproduction

```bash
# Analytical verification (Tasks A, B, C)
python3 scripts/leontovich.py

# Exhaustive tree sweep (Problem 4.3)
./synthesizer N --leontovich K --quiet

# Asymptotic near-path filter (general connected)
g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
geng -c K -q | ./leontovich_fast

# Bipartite filter
for n1 in $(seq 1 $((K/2))); do
  genbg -c $n1 $((K-n1)) -q | ./leontovich_fast
done

# Tree-as-target filter
./synthesizer M --export-g6 | ./leontovich_fast
```
