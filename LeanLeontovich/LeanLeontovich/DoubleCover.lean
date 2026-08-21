import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
The bipartite double cover is defined here rather than postulated. This file
proves the elementary lifting construction. The two-to-one counting identity
still requires a formal connected-bipartite source theorem.
-/

/- The bipartite double cover has two copies of each target vertex, and every
edge changes sides. -/
def DoubleCover (H : Graph) : Graph where
  V := H.V × Bool
  instFintype := inferInstance
  adj := fun u v => H.adj u.1 v.1 && decide (u.2 ≠ v.2)
  symm := by
    intro u v
    rw [H.symm]
    congr
    simp [ne_comm]

/- A proper two-coloring of a source graph. -/
structure BipartiteColoring (G : Graph) where
  color : G.V → Bool
  proper : ∀ ⦃u v : G.V⦄, G.adj u v = true → color u ≠ color v

/- Flip a Boolean side when requested. -/
def flipSide (side flip : Bool) : Bool :=
  if flip then !side else side

/- Every homomorphism from a two-colored source has two canonical lifts to the
double cover, one for each global choice of side. -/
noncomputable def liftHom {G H : Graph} (c : BipartiteColoring G)
    (f : {f : G.V → H.V // IsHom G H f}) (flip : Bool) :
    {g : G.V → (DoubleCover H).V // IsHom G (DoubleCover H) g} := by
  refine ⟨fun u => (f.1 u, flipSide (c.color u) flip), ?_⟩
  intro u v huv
  have hadj : H.adj (f.1 u) (f.1 v) = true := f.2 huv
  have hcolor : c.color u ≠ c.color v := c.proper huv
  simp only [DoubleCover]
  rw [hadj]
  simp [flipSide, hcolor]

/- Projecting a lifted homomorphism recovers its original target map. -/
theorem liftHom_projection {G H : Graph} (c : BipartiteColoring G)
    (f : {f : G.V → H.V // IsHom G H f}) (flip : Bool) (u : G.V) :
    (liftHom c f flip).1 u |>.1 = f.1 u := by
  rfl

end LeanLeontovich
