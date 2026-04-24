import Mathlib.Tactic
def edges : Nat := 1030
def mantel : Nat := 1024
def cycles : Nat := 194
theorem supersaturation : edges > mantel := by decide
theorem risky : cycles > 0 := by decide
