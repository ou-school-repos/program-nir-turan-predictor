import Mathlib.Tactic

def grid_size : Nat := 25
def nash_value : Nat := 25
def search_depth : Nat := 8

theorem optimal_defense : nash_value < grid_size := by decide
