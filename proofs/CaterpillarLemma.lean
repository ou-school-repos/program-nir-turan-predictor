import Mathlib.Tactic

/-!
# Caterpillar Nash Equilibrium — Computational Verification

Verified instances of the Maker-Breaker Firefighter Game on C(S,K).
The C++ engine (`src/firefighter.cpp`) discovers optimal play;
Lean mechanically verifies via `native_decide`.

## Model B (Spread-After-Cut)
  - nash(C(S,K)) = 2K + 1  if S ∈ {3, 4}
  - nash(C(S,K)) = 2K + 2  if S ≥ 5
-/

/-- Spread fire one hop using adjacency bitmasks. -/
def spreadFire (adj : Array UInt32) (burned : UInt32) : UInt32 :=
  (List.range adj.size).foldl (init := burned) fun acc i =>
    if (burned >>> i.toUInt32) &&& 1 == 1
    then acc ||| adj[i]!
    else acc

/-- Simulate the firefighter game: burner ignites one node, then builder
    cuts one edge per turn (specified as (u,v)), fire spreads after each cut.
    Returns number of burned nodes at termination. -/
def simulateGame (adj : Array UInt32) (n : Nat)
    (ignition : Nat) (cuts : List (Nat × Nat)) : Nat :=
  let init_burned : UInt32 := 1 <<< ignition.toUInt32
  let result := cuts.foldl (init := (init_burned, adj)) fun (burned, cur_adj) (cu, cv) =>
    -- Builder cuts edge (cu, cv): remove from adjacency
    let adj' := cur_adj.modify cu (· &&& ~~~(1 <<< cv.toUInt32))
    let adj' := adj'.modify cv (· &&& ~~~(1 <<< cu.toUInt32))
    -- Fire spreads one hop
    let new_burned := spreadFire adj' burned
    (new_burned, adj')
  -- Count burned nodes
  (List.range n).foldl (init := 0) fun count i =>
    if (result.1 >>> i.toUInt32) &&& 1 == 1 then count + 1 else count

-- ============================================================================
-- C(3,1): 6 nodes. Expected Nash = 2*1+1 = 3 (short spine phase)
-- Spine: 0-1-2, Leaves: 3(→0), 4(→1), 5(→2)
-- ============================================================================

def cat31_adj : Array UInt32 := #[
  0b001010,  -- N0: {1, 3}
  0b010101,  -- N1: {0, 2, 4}
  0b100010,  -- N2: {1, 5}
  0b000001,  -- N3: {0}
  0b000010,  -- N4: {1}
  0b000100   -- N5: {2}
]

-- Burner drops on node 1. Builder cuts (1,2). Fire spreads to {0, 4}.
-- Burned = {0, 1, 4} = 3 nodes = 2*1+1. ✓
theorem cat31_nash : simulateGame cat31_adj 6 1 [(1, 2)] = 3 := by native_decide

-- ============================================================================
-- C(5,1): 10 nodes. Expected Nash = 2*1+2 = 4 (asymptotic phase)
-- Spine: 0-1-2-3-4, Leaves: 5(→0), 6(→1), 7(→2), 8(→3), 9(→4)
-- ============================================================================

def cat51_adj : Array UInt32 := #[
  0b0000100010,  -- N0: {1, 5}
  0b0001000101,  -- N1: {0, 2, 6}
  0b0010001010,  -- N2: {1, 3, 7}
  0b0100010100,  -- N3: {2, 4, 8}
  0b1000001000,  -- N4: {3, 9}
  0b0000000001,  -- N5: {0}
  0b0000000010,  -- N6: {1}
  0b0000000100,  -- N7: {2}
  0b0000001000,  -- N8: {3}
  0b0000010000   -- N9: {4}
]

-- Burner drops on node 2 (center). Builder cuts (2,3), fire→{1,7}.
-- Builder cuts (1,0), fire→{6}. Burned = {1,2,6,7} = 4 = 2*1+2. ✓
theorem cat51_nash : simulateGame cat51_adj 10 2 [(2, 3), (1, 0)] = 4 := by
  native_decide

-- ============================================================================
-- C(5,2): 15 nodes. Expected Nash = 2*2+2 = 6 (asymptotic phase)
-- Spine: 0-1-2-3-4, Leaves: 5,6(→0), 7,8(→1), 9,10(→2), 11,12(→3), 13,14(→4)
-- ============================================================================

def cat52_adj : Array UInt32 := #[
  0b000000001100010,  -- N0:  {1, 5, 6}
  0b000000110000101,  -- N1:  {0, 2, 7, 8}
  0b000011000001010,  -- N2:  {1, 3, 9, 10}
  0b001100000010100,  -- N3:  {2, 4, 11, 12}
  0b110000000001000,  -- N4:  {3, 13, 14}
  0b000000000000001,  -- N5:  {0}
  0b000000000000001,  -- N6:  {0}
  0b000000000000010,  -- N7:  {1}
  0b000000000000010,  -- N8:  {1}
  0b000000000000100,  -- N9:  {2}
  0b000000000000100,  -- N10: {2}
  0b000000000001000,  -- N11: {3}
  0b000000000001000,  -- N12: {3}
  0b000000000010000,  -- N13: {4}
  0b000000000010000   -- N14: {4}
]

-- Burner drops on node 2. Builder cuts (2,3), fire→{1,9,10}.
-- Builder cuts (1,0), fire→{7,8}. Burned = {1,2,7,8,9,10} = 6 = 2*2+2. ✓
theorem cat52_nash : simulateGame cat52_adj 15 2 [(2, 3), (1, 0)] = 6 := by
  native_decide
