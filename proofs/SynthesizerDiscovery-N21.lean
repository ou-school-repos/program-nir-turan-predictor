import Mathlib.Tactic
def synth_n : Nat := 21
def path_score : Nat := 28657
def synth_score : Nat := 1048577
def trees_scanned : Nat := 35221832
theorem anomaly_discovered : synth_score > path_score := by decide
