import Mathlib.Tactic

def grid_size : Nat := 16
def nash_value : Nat := 4
def search_depth : Nat := 8

theorem optimal_defense : nash_value < grid_size := by decide
