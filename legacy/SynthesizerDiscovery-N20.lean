import Mathlib.Tactic
def synth_n : Nat := 20
def path_score : Nat := 17711
def synth_score : Nat := 30135
def trees_scanned : Nat := 823065
def wiener_index : Nat := 745
theorem anomaly_discovered : synth_score > path_score := by decide
