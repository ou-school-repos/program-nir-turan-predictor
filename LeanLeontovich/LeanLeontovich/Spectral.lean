import LeanLeontovich.Basic

namespace LeanLeontovich

/-!
Spectral-theoretic statements for the Leontovich setting.

This file records the main obstruction theorem from the paper. The final
`spectral_obstruction` theorem depends on the explicit quotient-matrix
certificate packaged in `single_positive_eigenvalue_obstruction`. The
spectral witness itself is still an unproven axiom in this file.
-/

/-- Unproven quotient-matrix witness for `HStar`. This remains a trust
assumption until the spectral certificate is formalized directly in Lean. -/
axiom HStar_quotient_matrix_has_one_positive_eigenvalue : Prop

theorem spectral_obstruction : ¬ IsLeontovich HStar := by
  let hCert : SinglePositiveEigenvalueCertificate HStar := {
    quotientMatrixHasOnePositiveEigenvalue :=
      HStar_quotient_matrix_has_one_positive_eigenvalue
  }
  exact single_positive_eigenvalue_obstruction hCert

end LeanLeontovich
