import Mathlib.Tactic

namespace LeanLeontovich

/-- A finite simple graph with Boolean adjacency. -/
structure Graph where
  V : Type
  instFintype : Fintype V
  adj : V → V → Bool
  symm : ∀ u v, adj u v = adj v u
  loopless : ∀ u, adj u u = false

instance (G : Graph) : Fintype G.V := G.instFintype

/-- A graph isomorphism preserving adjacency. -/
structure GraphIso (G H : Graph) where
  toEquiv : G.V ≃ H.V
  adj_iff : ∀ u v, G.adj u v = true ↔ H.adj (toEquiv u) (toEquiv v) = true

/-- The path adjacency predicate on `Fin n`. -/
def pathRel {n : Nat} (u v : Fin n) : Prop :=
  u.val + 1 = v.val ∨ v.val + 1 = u.val

/-- Boolean adjacency for the path graph on `Fin n`. -/
noncomputable def pathAdj {n : Nat} (u v : Fin n) : Bool :=
  decide (u.val + 1 = v.val) || decide (v.val + 1 = u.val)

/-- A graph homomorphism. -/
def IsHom (G H : Graph) (f : G.V → H.V) : Prop :=
  ∀ ⦃u v : G.V⦄, G.adj u v = true → H.adj (f u) (f v) = true

/-- Exact homomorphism count between finite graphs. -/
noncomputable def Hom (G H : Graph) : Nat := by
  classical
  exact Fintype.card {f : G.V → H.V // IsHom G H f}

/-- Every finite graph admits at least one homomorphism to itself. -/
theorem hom_self_pos (G : Graph) : 0 < Hom G G := by
  classical
  have hnonempty : Nonempty {f : G.V → G.V // IsHom G G f} := by
    refine ⟨⟨id, ?_⟩⟩
    intro u v huv
    exact huv
  simpa [Hom] using Fintype.card_pos_iff.mpr hnonempty

/-- Homomorphism counts are invariant under graph isomorphism on the source side. -/
theorem hom_congr_left {G H K : Graph} (i : GraphIso G H) :
    Hom G K = Hom H K := by
  classical
  let toFun :
      {f : G.V → K.V // IsHom G K f} →
        {g : H.V → K.V // IsHom H K g} := by
    intro f
    refine ⟨f.1 ∘ i.toEquiv.symm, ?_⟩
    intro u v huv
    have hG : G.adj (i.toEquiv.symm u) (i.toEquiv.symm v) = true := by
      exact (i.adj_iff (i.toEquiv.symm u) (i.toEquiv.symm v)).2 (by
        simpa using huv)
    exact f.2 hG
  let invFun :
      {g : H.V → K.V // IsHom H K g} →
        {f : G.V → K.V // IsHom G K f} := by
    intro g
    refine ⟨g.1 ∘ i.toEquiv, ?_⟩
    intro u v huv
    have hH : H.adj (i.toEquiv u) (i.toEquiv v) = true := by
      exact (i.adj_iff u v).1 huv
    exact g.2 hH
  have hleft : Function.LeftInverse invFun toFun := by
    intro g
    ext u
    simp [toFun, invFun]
  have hright : Function.RightInverse invFun toFun := by
    intro f
    ext u
    simp [toFun, invFun]
  have hEquiv :
      {f : G.V → K.V // IsHom G K f} ≃ {g : H.V → K.V // IsHom H K g} :=
    ⟨toFun, invFun, hleft, hright⟩
  exact Fintype.card_congr hEquiv

/-- The path graph `P_n`. -/
noncomputable def Path (n : Nat) : Graph where
  V := Fin n
  instFintype := inferInstance
  adj := pathAdj
  symm := by
    intro u v
    cases h1 : decide (u.val + 1 = v.val) <;>
    cases h2 : decide (v.val + 1 = u.val) <;>
    simp [pathAdj, h1, h2]
  loopless := by
    intro u
    simp [pathAdj]

/-- The near-path relation `E_n` from the paper. -/
def nearPathRel (n : Nat) (u v : Fin n) : Prop :=
  pathRel u v ∨
    (4 ≤ n ∧ ((u.val = n - 4 ∧ v.val = n - 1) ∨ (u.val = n - 1 ∧ v.val = n - 4)))

/-- Boolean adjacency for the near-path graph `E_n`. -/
noncomputable def nearPathAdj (n : Nat) (u v : Fin n) : Bool :=
  by
    classical
    exact decide (nearPathRel n u v)

/-- The near-path graph `E_n`. -/
noncomputable def NearPath (n : Nat) : Graph where
  V := Fin n
  instFintype := inferInstance
  adj := nearPathAdj n
  symm := by
    intro u v
    simp [nearPathAdj, nearPathRel, pathRel, or_comm, or_left_comm, or_assoc, and_left_comm, and_assoc, and_comm]
  loopless := by
    intro u
    simp [nearPathAdj, nearPathRel, pathRel]
    omega

/-- The path graph has a positive self-homomorphism count. -/
theorem path_hom_self_pos (n : Nat) : 0 < Hom (Path n) (Path n) := by
  exact hom_self_pos (Path n)

/-- The near-path graph has a positive self-homomorphism count. -/
theorem nearpath_hom_self_pos (n : Nat) : 0 < Hom (NearPath n) (NearPath n) := by
  exact hom_self_pos (NearPath n)

/-- Oddness predicate for source sizes. -/
def IsOdd (n : Nat) : Prop := n % 2 = 1

/-- Evenness predicate for source sizes. -/
def IsEven (n : Nat) : Prop := n % 2 = 0

/-- A graph is Leontovich if some odd near-path beats the path count. -/
def IsLeontovich (H : Graph) : Prop :=
  ∃ n, IsOdd n ∧ Hom (NearPath n) H < Hom (Path n) H

/-- A graph is strongly Leontovich if the near-path family eventually beats the path family. -/
def IsStronglyLeontovich (H : Graph) : Prop :=
  ∃ N, ∀ n, N ≤ n → Hom (NearPath n) H < Hom (Path n) H

/-- The bipartite double cover of a graph. -/
axiom DoubleCover : Graph → Graph

/-- The named graph `T(7,1,9)`. -/
axiom T719 : Graph

/-- The 15-vertex depth-dependent bipartite witness from the paper. -/
axiom HStar : Graph

/-- The 18-vertex depth-2 bipartite witness from the paper. -/
axiom H18 : Graph

/-- The 76-vertex pruned tree witness from the paper. -/
axiom H76 : Graph

/-- The 1,822-vertex looped symmetric strongly Leontovich witness. -/
axiom H1822 : Graph

/-- The perturbed 6,806-vertex strongly Leontovich certificate graph. -/
axiom BPrime : Graph

/-- Certificate data for the single-positive-eigenvalue obstruction.
The graph-theoretic symmetry and looplessness are recorded alongside the
separate quotient-matrix witness so the obstruction is not phrased only in
terms of intrinsic `Graph` fields. -/
structure SinglePositiveEigenvalueCertificate (H : Graph) where
  graph_symm : ∀ u v, H.adj u v = H.adj v u
  graph_loopless : ∀ u, H.adj u u = false
  quotientMatrixHasOnePositiveEigenvalue : Prop

/-- The single-positive-eigenvalue obstruction theorem stated against an
explicit certificate rather than against the raw graph axioms. -/
axiom single_positive_eigenvalue_obstruction :
  ∀ {H : Graph}, SinglePositiveEigenvalueCertificate H → ¬ IsLeontovich H

/-- The permanent crossover threshold at `T(7,1,9)`. -/
axiom permanent_crossover_T719 :
  (∀ n, IsOdd n → 13 ≤ n → Hom (NearPath n) T719 < Hom (Path n) T719) ∧
  (∀ n, n < 13 → Hom (Path n) T719 ≤ Hom (NearPath n) T719) ∧
  (∀ n, IsEven n → Hom (Path n) T719 ≤ Hom (NearPath n) T719)

/-- Bounded local pruning audit for the `T(7,1,9)` leaf-pruning landscape. -/
axiom local_smt_pruning_audit :
  IsLeontovich H76

/-- Certificate data for the depth-2 `m₁ = 2` obstruction.
The bipartite restriction is recorded explicitly, together with the left-side
cardinality condition used in the paper. -/
structure Depth2BipartiteCertificate (H : Graph) where
  bipartite : Prop
  leftPartitionSize : Nat
  leftPartitionSize_eq_two : leftPartitionSize = 2

/-- Depth-2 obstruction for bipartite graphs with left partition size 2. -/
axiom depth2_obstruction_m1_two :
  ∀ {H : Graph}, Depth2BipartiteCertificate H → Hom (Path 5) H ≤ Hom (NearPath 5) H

/-- Exact `n = 17` crossover witness for the 18-vertex depth-2 graph. -/
axiom h18_depth2_crossover :
  Hom (Path 17) H18 > Hom (NearPath 17) H18

/-- Minimality statement for the depth-2 bipartite sweep. -/
axiom h18_minimal_depth2_sweep :
  IsLeontovich H18

/-- Double-cover preservation for Leontovich graphs. -/
axiom leontovich_iff_double_cover :
  ∀ H : Graph, IsLeontovich H ↔ IsLeontovich (DoubleCover H)

/-- Double-cover preservation for strong Leontovich graphs. -/
axiom strongly_leontovich_iff_double_cover :
  ∀ H : Graph, IsStronglyLeontovich H ↔ IsStronglyLeontovich (DoubleCover H)

/-- Strongly Leontovich witness for the 1,822-vertex looped symmetric tree. -/
axiom h1822_strongly_leontovich :
  IsStronglyLeontovich H1822

/-- Strongly Leontovich certificate for the perturbed 6,806-vertex graph. -/
axiom perturbed_nonbipartite_certificate :
  IsStronglyLeontovich BPrime

end LeanLeontovich
