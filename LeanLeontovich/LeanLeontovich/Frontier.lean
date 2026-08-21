import LeanLeontovich.Basic

namespace LeanLeontovich.Assumed

/-!
Structural frontier results for the smallest Leontovich witnesses.

This file records assumed witness status separately from bounded sweep
minimality. The local SMT pruning experiment times out and therefore has no
theorem or axiom here.
-/

theorem h18_witness_certificate : IsLeontovich H18 := by
  exact h18_is_leontovich

end LeanLeontovich.Assumed
