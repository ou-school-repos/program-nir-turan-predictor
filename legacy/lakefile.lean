import Lake
open Lake DSL

package «proofs» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "028964f2c6f063a423db7395fb84ffc2abc197bb"

@[default_target]
lean_lib «ProgramNir» where
  srcDir := "."
  roots := #[
    `Adversarial,
    `SolverVerification,
    `VectorDeployment,
    `ThreatHunting,
    `SignalAudit,
    `RiskAudit,
    `CaterpillarLemma,
    Lean.Name.str Lean.Name.anonymous "SynthesizerDiscovery-N15",
    Lean.Name.str Lean.Name.anonymous "SynthesizerDiscovery-N20",
    Lean.Name.str Lean.Name.anonymous "SynthesizerDiscovery-N21"
  ]
