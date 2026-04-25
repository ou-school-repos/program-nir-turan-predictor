# Adversarial Burning Game: Caterpillar Nash Equilibrium

## Overview

We study the **Maker-Breaker burning game** on generalized caterpillar trees
$C(S,K)$, where $S$ is the spine length (backbone) and $K$ is the number of
pendant legs per spine node. Total vertices: $N = S(K+1)$.

- **Burner** (attacker) drops fire sources to maximize destruction
- **Builder** (defender) severs one edge per turn to contain the spread
- **Nash value**: minimax equilibrium -- max nodes burner can guarantee

## Two Game Models

The Nash equilibrium depends critically on the **timing model** of fire
spread relative to the builder's action.

### Model A: Alternating Plies (Asynchronous Response)

Burner and Builder alternate turns. Fire does **not** spread during the
Builder's turn. This models a defender who can react _before_ the next
propagation cycle (e.g., real-time network segmentation).

**Result:** For $S \geq K+2$:

$$\text{nash}_A(C(S,K)) = K + 2$$

### Model B: Spread-After-Cut (Synchronous Response)

Each round: Builder cuts one edge, then fire spreads one hop. Fire
propagates _in the same turn_ as the builder's action. This models
the standard firefighter game where the defender and the fire act
simultaneously.

**Result:** For $S \geq 5$:

$$\text{nash}_B(C(S,K)) = 2K + 2$$

For $S \in \{3, 4\}$: $\text{nash}_B = 2K + 1$.

### Comparison

| Property               | Model A (Async)       | Model B (Sync)       |
| ---------------------- | --------------------- | -------------------- |
| Formula ($S$ large)    | $K + 2$               | $2K + 2$             |
| Short-spine correction | $S \geq K+2$          | $S \geq 5$           |
| Fire timing            | Between builder turns | Same turn as cut     |
| Damage ratio           | $(K+2)/N \to 0$       | $(2K+2)/N \to 0$     |
| Interpretation         | Fast responder        | Standard firefighter |

Both models share a key structural property: **the Nash value is
independent of spine length** once $S$ exceeds a small threshold.

## Computational Evidence

### Model A (Alternating Plies)

Computed via Alpha-Beta search with Zobrist transposition tables
(`src/dendro.cpp`).

| $K$      | Nash | $K+2$ | Verified Range |
| -------- | ---- | ----- | -------------- |
| 0 (path) | 2    | 2     | $S=3..12$      |
| 1        | 3    | 3     | $S=3..12$      |
| 2        | 4    | 4     | $S=4..10$      |
| 3        | 5    | 5     | $S=3..8$       |
| 4        | 6    | 6     | $S=4..6$       |

### Model B (Spread-After-Cut)

Computed via single-ignition recursive Minimax with memoization
(`src/firefighter.cpp`).

| $S \setminus K$ | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   |
| --------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3               | 2   | 3   | 5   | 7   | 9   | 11  | 13  | 15  | 17  |
| 4               | 2   | 3   | 5   | 7   | 9   | 11  | 13  | 15  | 17  |
| 5               | 2   | 4   | 6   | 8   | 10  | 12  | 14  | 16  | 18  |
| 6               | 2   | 4   | 6   | 8   | 10  | 12  | 14  | 16  | 18  |
| 7               | 2   | 4   | 6   | 8   | 10  | 12  | 14  | 16  | 18  |
| 8               | 2   | 4   | 6   | 8   | 10  | 12  | 14  | --  | --  |

Short-spine phase ($S \leq 4$): $2K+1$. Asymptotic phase ($S \geq 5$): $2K+2$.

## Proof Sketch

### Model A: Why $K+2$?

The burner drops on a spine node $v$ with degree $K+1$ ($K$ legs + 1 spine
neighbor). The builder severs one spine edge, isolating the fire to $v$, its
$K$ legs, and one spine neighbor. The builder always has a free action to cut
the remaining spine edge before the next spread cycle.

### Model B: Why $2K+2$?

The burner drops on a central spine node $v_i$. The builder cuts
$(v_i, v_{i+1})$. Fire simultaneously spreads to $v_{i-1}$ and $K$ legs
of $v_i$. On turn 2, the builder _must_ cut $(v_{i-1}, v_{i-2})$ to prevent
longitudinal escape, but fire simultaneously reaches the $K$ legs of
$v_{i-1}$. Total: $v_i + v_{i-1} + 2K$ legs $= 2K + 2$.

For $S \leq 4$, the fire hits the spine endpoint, relieving the builder from
one spine cut. The saved action severs a leaf edge, reducing damage to $2K+1$.

## Applications

### Telecom Network Resilience

- **Spine** = fiber backbone between cell towers
- **Legs** = access point connections per tower ($K$ per hub)
- **Model A** (fast segmentation): damage bounded by hub degree $K+2$
- **Model B** (standard response): damage bounded by $2K+2$
- **Key insight**: backbone length does not affect vulnerability

### Drone Patrol Corridor Security

- **Spine** = patrol corridor waypoints
- **Legs** = observation post links per waypoint ($K$ per waypoint)
- **Implication**: a saboteur cutting communication links can contain
  any intrusion to $K+2$ (fast response) or $2K+2$ (standard response)
  sectors, regardless of corridor length

## Reproduction

```bash
# Model A sweep (alternating plies)
make dendro
bash scripts/sweep_caterpillar.sh
python3 scripts/analyze_caterpillar.py

# Model B (single-ignition firefighter)
make firefighter
./firefighter
```

## Data

Raw sweep data: `docs/runs/caterpillar_nash.csv`
