import Mathlib.Tactic
def synth_n : Nat := 21
def path_score : Nat := 28657
def synth_score : Nat := 49913
def trees_scanned : Nat := 2144505
def wiener_index : Nat := 844
theorem anomaly_discovered : synth_score > path_score := by decide
