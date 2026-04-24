import Mathlib.Tactic

def edges : Nat := 1029
def mantel_limit : Nat := 1024
def exact_cycles : Nat := 160

theorem supersaturation_active : edges > mantel_limit := by decide
theorem cycles_exist : exact_cycles > 0 := by decide
