import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
Crossover theorems for specific target graphs and source families.

The theorem statement below packages the `T(7,1,9)` threshold theorem in the
form used throughout the paper. It is a direct wrapper around
`permanent_crossover_T719`.
-/

theorem T719_permanent_crossover :
    (∀ n, IsOdd n → 13 ≤ n → Hom (NearPath n) T719 < Hom (Path n) T719) ∧
    (∀ n, n < 13 → Hom (Path n) T719 ≤ Hom (NearPath n) T719) ∧
    (∀ n, IsEven n → Hom (Path n) T719 ≤ Hom (NearPath n) T719) := by
  exact permanent_crossover_T719

end LeanLeontovich
