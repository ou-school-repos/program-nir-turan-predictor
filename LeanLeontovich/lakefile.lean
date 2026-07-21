import Lake
open Lake DSL

package «LeanLeontovich» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "master"

@[default_target]
lean_lib «LeanLeontovich» where
  srcDir := "."
  roots := #[
    `LeanLeontovich.Main,
    `LeanLeontovich.Basic,
    `LeanLeontovich.Spectral,
    `LeanLeontovich.Crossover,
    `LeanLeontovich.Frontier,
    `LeanLeontovich.DoubleCover
  ]
