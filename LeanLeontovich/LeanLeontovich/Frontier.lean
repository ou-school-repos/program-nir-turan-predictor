import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
Structural frontier results for the smallest Leontovich witnesses.

This file is the home for the bounded SMT audit and the minimality claims for
the pruning landscape and the depth-2 sweep. The named theorems below are
thin wrappers over `local_smt_pruning_audit` and `h18_minimal_depth2_sweep`.
-/

theorem pruning_audit_certificate : IsLeontovich H76 := by
  exact local_smt_pruning_audit

theorem h18_minimality_certificate : IsLeontovich H18 := by
  exact h18_minimal_depth2_sweep

end LeanLeontovich
