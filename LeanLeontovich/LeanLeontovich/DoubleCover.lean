import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
Results about bipartite double covers and strong Leontovich behavior.

The double-cover theorem is the bridge between ordinary Leontovich graphs and
simple bipartite strong-Leontovich witnesses.
-/

theorem double_cover_preserves_leontovich :
    ∀ H : Graph, IsLeontovich H ↔ IsLeontovich (DoubleCover H) := by
  exact leontovich_iff_double_cover

theorem double_cover_preserves_strongly_leontovich :
    ∀ H : Graph, IsStronglyLeontovich H ↔ IsStronglyLeontovich (DoubleCover H) := by
  exact strongly_leontovich_iff_double_cover

end LeanLeontovich
