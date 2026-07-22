import LeanLeontovich.Core

namespace LeanLeontovich

/-!
Basic scaffolding shared by the analytic theorem files.

The intention here is to collect the abstract graph and homomorphism
conventions needed by the main theorems, without importing the witness-centric
definitions from `legacy/`.
-/

theorem nat_two_pos : 0 < 2 := by
  decide

end LeanLeontovich
