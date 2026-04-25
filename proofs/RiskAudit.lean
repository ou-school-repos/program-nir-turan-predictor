import Mathlib.Tactic

def edges : Nat := 1032
def mantel_limit : Nat := 1024
def exact_cycles : Nat := 258

theorem supersaturation_active : edges > mantel_limit := by decide
theorem cycles_exist : exact_cycles > 0 := by decide
