import Mathlib.Tactic

def path_allocations : Nat := 196418
def discovered_allocations : Nat := 729216

theorem anomaly_verified : discovered_allocations > path_allocations := by decide
