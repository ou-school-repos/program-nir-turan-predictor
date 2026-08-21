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
    cases u.2 <;> cases v.2 <;> rfl

/- A proper two-coloring of a source graph. -/
structure BipartiteColoring (G : Graph) where
  color : G.V → Bool
  proper : ∀ ⦃u v : G.V⦄, G.adj u v = true → color u ≠ color v

/- Reachability from a chosen root, expressed directly for the repository's
Boolean-adjacency graph model. -/
inductive ReachableFrom (G : Graph) (root : G.V) : G.V → Prop where
  | root : ReachableFrom G root root
  | step {u v : G.V} : ReachableFrom G root u → G.adj u v = true →
      ReachableFrom G root v

/- A proper coloring together with a concrete connectedness witness. -/
structure ConnectedBipartiteColoring (G : Graph) extends BipartiteColoring G where
  root : G.V
  reachable : ∀ v : G.V, ReachableFrom G root v

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
  cases hcu : c.color u <;> cases hcv : c.color v <;>
    simp_all [flipSide]

/- Projecting a lifted homomorphism recovers its original target map. -/
theorem liftHom_projection {G H : Graph} (c : BipartiteColoring G)
    (f : {f : G.V → H.V // IsHom G H f}) (flip : Bool) (u : G.V) :
    ((liftHom c f flip).1 u).1 = f.1 u := by
  rfl

/- Forgetting the side of a double-cover homomorphism gives a homomorphism to
the original target. -/
def projectHom {G H : Graph}
    (g : {g : G.V → (DoubleCover H).V // IsHom G (DoubleCover H) g}) :
    {f : G.V → H.V // IsHom G H f} := by
  refine ⟨fun u => (g.1 u).1, ?_⟩
  intro u v huv
  have hcover := g.2 huv
  simp only [DoubleCover, Bool.and_eq_true] at hcover
  exact hcover.1

/- Projection is a left inverse to either canonical lift. -/
theorem projectHom_liftHom {G H : Graph} (c : BipartiteColoring G)
    (f : {f : G.V → H.V // IsHom G H f}) (flip : Bool) :
    projectHom (liftHom c f flip) = f := by
  ext u
  rfl

/- On a nonempty source, the target map and global side choice uniquely
determine a canonical lift. -/
theorem liftHom_pair_injective {G H : Graph} [Nonempty G.V]
    (c : BipartiteColoring G) :
    Function.Injective (fun p : {f : G.V → H.V // IsHom G H f} × Bool =>
      liftHom c p.1 p.2) := by
  intro p q hpq
  have hmaps : p.1 = q.1 := by
    have hprojected := congrArg projectHom hpq
    simpa only [projectHom_liftHom] using hprojected
  have hsides : p.2 = q.2 := by
    let u : G.V := Classical.choice (inferInstance : Nonempty G.V)
    have hvalue : flipSide (c.color u) p.2 = flipSide (c.color u) q.2 := by
      have := congrArg (fun g => (g.1 u).2) hpq
      simpa only [liftHom] using this
    cases hc : c.color u <;> cases hp : p.2 <;> cases hq : q.2 <;>
      simp_all [flipSide]
  exact Prod.ext hmaps hsides

/- This is the exact structural obligation supplied by connectedness: every
double-cover map has one global choice of side relative to the coloring. -/
def EveryDoubleCoverHomLifts {G : Graph} (c : BipartiteColoring G) : Prop :=
  ∀ (H : Graph) (g : {g : G.V → (DoubleCover H).V // IsHom G (DoubleCover H) g}),
    ∃ flip : Bool, g = liftHom c (projectHom g) flip

/- Along a path from the root, properness of both colorings forces the side of
the double-cover map to remain the same global flip of the source coloring. -/
theorem side_eq_flipSide_of_reachable {G H : Graph}
    (c : ConnectedBipartiteColoring G)
    (g : {g : G.V → (DoubleCover H).V // IsHom G (DoubleCover H) g})
    (v : G.V) :
    (g.1 v).2 = flipSide (c.color v)
      (decide (c.color c.root ≠ (g.1 c.root).2)) := by
  let flip := decide (c.color c.root ≠ (g.1 c.root).2)
  have hroot : (g.1 c.root).2 = flipSide (c.color c.root) flip := by
    dsimp [flip]
    cases hc : c.color c.root <;> cases hg : (g.1 c.root).2 <;>
      simp [flipSide]
  have propagate : ∀ {u w : G.V}, ReachableFrom G c.root u →
      (g.1 u).2 = flipSide (c.color u) flip → G.adj u w = true →
      (g.1 w).2 = flipSide (c.color w) flip := by
    intro u w _ hu huw
    have hg := g.2 huw
    have hc := c.proper huw
    simp only [DoubleCover, Bool.and_eq_true] at hg
    have hsides : (g.1 u).2 ≠ (g.1 w).2 := by
      exact of_decide_eq_true hg.2
    cases hcu : c.color u <;> cases hcw : c.color w <;>
      cases hgu : (g.1 u).2 <;> cases hgw : (g.1 w).2 <;>
      cases hf : flip <;> simp_all [flipSide]
  induction c.reachable v with
  | root => exact hroot
  | step hu huw ih => exact propagate hu ih huw

/- Connectedness supplies the structural obligation used by `liftHomEquiv`. -/
theorem connected_everyDoubleCoverHomLifts {G : Graph}
    (c : ConnectedBipartiteColoring G) :
    EveryDoubleCoverHomLifts c.toBipartiteColoring := by
  intro H g
  let flip := decide (c.color c.root ≠ (g.1 c.root).2)
  refine ⟨flip, ?_⟩
  apply Subtype.ext
  funext v
  apply Prod.ext
  · rfl
  · dsimp [liftHom, projectHom, flip]
    exact side_eq_flipSide_of_reachable c g v

/- Once the structural obligation is available, canonical lifting is an
equivalence rather than merely an injection. -/
noncomputable def liftHomEquiv {G H : Graph} [Nonempty G.V]
    (c : BipartiteColoring G) (hall : EveryDoubleCoverHomLifts c) :
    ({f : G.V → H.V // IsHom G H f} × Bool) ≃
      {g : G.V → (DoubleCover H).V // IsHom G (DoubleCover H) g} := by
  apply Equiv.ofBijective
    (fun p : {f : G.V → H.V // IsHom G H f} × Bool => liftHom c p.1 p.2)
  refine ⟨liftHom_pair_injective c, ?_⟩
  intro g
  obtain ⟨flip, hflip⟩ := hall H g
  exact ⟨(projectHom g, flip), hflip.symm⟩

/- The double-cover counting identity now depends only on the connected-source
structural obligation, isolated above. -/
theorem hom_doubleCover_eq_two_mul {G H : Graph} [Nonempty G.V]
    (c : BipartiteColoring G) (hall : EveryDoubleCoverHomLifts c) :
    Hom G (DoubleCover H) = 2 * Hom G H := by
  classical
  have hcard := Fintype.card_congr (liftHomEquiv (H := H) c hall)
  simpa [Hom, Nat.mul_comm] using hcard.symm

/- The combinatorial double-cover identity for a connected bipartite source. -/
theorem connected_hom_doubleCover_eq_two_mul {G H : Graph}
    (c : ConnectedBipartiteColoring G) :
    Hom G (DoubleCover H) = 2 * Hom G H := by
  letI : Nonempty G.V := ⟨c.root⟩
  exact hom_doubleCover_eq_two_mul c.toBipartiteColoring
    (connected_everyDoubleCoverHomLifts c)

end LeanLeontovich
