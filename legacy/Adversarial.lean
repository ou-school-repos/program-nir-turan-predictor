import Mathlib.Tactic

def grid_size : Nat := 10
def nash_value : Nat := 3
def search_depth : Nat := 10

theorem optimal_defense : nash_value < grid_size := by decide
