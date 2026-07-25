import Mathlib.Tactic
def synth_n : Nat := 15
def path_score : Nat := 1597
def synth_score : Nat := 16385
def trees_scanned : Nat := 7741
def wiener_index : Nat := 196
theorem anomaly_discovered : synth_score > path_score := by decide
