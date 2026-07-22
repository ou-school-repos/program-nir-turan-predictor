import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
Spectral-theoretic statements for the Leontovich setting.

This file records the main obstruction theorem from the paper: a quotient
matrix with a single positive eigenvalue cannot support a Leontovich target.
-/

axiom spectral_obstruction : ¬ IsLeontovich HStar

end LeanLeontovich
