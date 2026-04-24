import Mathlib.Tactic

def path_allocations : Nat := 3145728
def leontovich_allocations : Nat := 3145728

theorem anomaly_verified : leontovich_allocations < path_allocations := by decide
