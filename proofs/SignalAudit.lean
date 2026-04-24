import Mathlib.Tactic
def path_allocations : Nat := 28657
def leontovich_allocations : Nat := 525313
theorem anomaly_verified : leontovich_allocations < path_allocations := by decide
