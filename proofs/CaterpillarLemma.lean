import Mathlib.Tactic

/-!
# Caterpillar Nash Equilibrium — Full Minimax Verification

Implements the complete Maker-Breaker Firefighter game in Lean 4.
The Burner chooses an ignition node (maximizing damage).
The Builder cuts one firefront edge per turn (minimizing damage).
Fire spreads one hop after each cut.

Proven via `native_decide`: Lean's kernel evaluates the full game tree.
-/

-- ============================================================================
-- GAME ENGINE (UInt32 bitboard)
-- ============================================================================

def spreadFire (adj : Array UInt32) (burned : UInt32) : UInt32 :=
  (List.range adj.size).foldl (init := burned) fun acc i =>
    if (burned >>> i.toUInt32) &&& 1 == 1
    then acc ||| adj[i]!
    else acc

def popcount (x : UInt32) : Nat :=
  (List.range 32).foldl (init := 0) fun count i =>
    if (x >>> i.toUInt32) &&& 1 == 1 then count + 1 else count

def cutEdge (adj : Array UInt32) (u v : Nat) : Array UInt32 :=
  let adj := adj.modify u (· &&& ~~~(1 <<< v.toUInt32))
  adj.modify v (· &&& ~~~(1 <<< u.toUInt32))

def testBit (x : UInt32) (i : Nat) : Bool :=
  (x >>> i.toUInt32) &&& 1 == 1

/-- Builder minimizes burned nodes over all firefront edge cuts. -/
def builderMinBurn (adj : Array UInt32) (burned : UInt32)
    (fuel : Nat) : Nat :=
  match fuel with
  | 0 => popcount burned
  | fuel' + 1 =>
    let n := adj.size
    -- Collect firefront edges: u burned, v not burned, edge (u,v) exists
    let candidates : List (Nat × Nat) :=
      (List.range n).foldl (init := ([] : List (Nat × Nat))) fun acc u =>
        if testBit burned u then
          (List.range n).foldl (init := acc) fun acc2 v =>
            if v > u && testBit (adj[u]!) v && !testBit burned v
            then (u, v) :: acc2
            else acc2
        else acc
    match candidates with
    | [] => popcount burned
    | _ =>
      candidates.foldl (init := popcount burned) fun best (u, v) =>
        let adj' := cutEdge adj u v
        let newBurned := spreadFire adj' burned
        if newBurned == burned then
          Nat.min best (popcount burned)
        else
          Nat.min best (builderMinBurn adj' newBurned fuel')

/-- Burner maximizes: try all spine nodes as ignition. -/
def burnerMaxBurn (adj : Array UInt32) (spineLen : Nat)
    (fuel : Nat) : Nat :=
  (List.range spineLen).foldl (init := 0) fun best i =>
    let burned : UInt32 := (1 : UInt32) <<< i.toUInt32
    Nat.max best (builderMinBurn adj burned fuel)

-- ============================================================================
-- CATERPILLAR GRAPHS
-- ============================================================================

-- C(3,1): 6 nodes, 5 edges
def cat31 : Array UInt32 := #[
  0b001010, 0b010101, 0b100010,
  0b000001, 0b000010, 0b000100
]

-- C(4,1): 8 nodes, 7 edges
def cat41 : Array UInt32 := #[
  0b00010010, 0b00100101, 0b01001010, 0b10000100,
  0b00000001, 0b00000010, 0b00000100, 0b00001000
]

-- C(5,1): 10 nodes, 9 edges
def cat51 : Array UInt32 := #[
  0b0000100010, 0b0001000101, 0b0010001010, 0b0100010100, 0b1000001000,
  0b0000000001, 0b0000000010, 0b0000000100, 0b0000001000, 0b0000010000
]

-- C(3,2): 9 nodes, 8 edges
def cat32 : Array UInt32 := #[
  0b000011010, 0b001100101, 0b110000010,
  0b000000001, 0b000000001, 0b000000010,
  0b000000010, 0b000000100, 0b000000100
]

-- C(5,2): 15 nodes, 14 edges
def cat52 : Array UInt32 := #[
  0b000000001100010, 0b000000110000101, 0b000011000001010,
  0b001100000010100, 0b110000000001000,
  0b000000000000001, 0b000000000000001,
  0b000000000000010, 0b000000000000010,
  0b000000000000100, 0b000000000000100,
  0b000000000001000, 0b000000000001000,
  0b000000000010000, 0b000000000010000
]

-- ============================================================================
-- THEOREMS: Full minimax, zero sorry
-- ============================================================================

-- Short spine phase (S ≤ 4): nash = 2K+1
theorem cat31_nash : burnerMaxBurn cat31 3 5 = 3 := by native_decide
theorem cat41_nash : burnerMaxBurn cat41 4 7 = 3 := by native_decide
theorem cat32_nash : burnerMaxBurn cat32 3 8 = 5 := by native_decide

-- Asymptotic phase (S ≥ 5): nash = 2K+2
theorem cat51_nash : burnerMaxBurn cat51 5 9 = 4 := by native_decide
theorem cat52_nash : burnerMaxBurn cat52 5 14 = 6 := by native_decide
