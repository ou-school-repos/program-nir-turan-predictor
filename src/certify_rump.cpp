#include <flint/acb.h>
#include <flint/acb_mat.h>
#include <flint/arb.h>
#include <flint/flint.h>

#include <iostream>

namespace {

constexpr slong kPrecision = 128;
constexpr slong kDimension = 2;
constexpr ulong kDepth = 2;

bool contains_real_value(const acb_t value, slong expected) {
    return arb_contains_si(acb_realref(value), expected) &&
           arb_contains_zero(acb_imagref(value));
}

}  // namespace

int main() {
    // Reversible quotient with Perron eigenpair lambda = 2, u = (1, 1).
    acb_mat_t matrix;
    acb_mat_init(matrix, kDimension, kDimension);
    acb_set_si(acb_mat_entry(matrix, 0, 1), 2);
    acb_set_si(acb_mat_entry(matrix, 1, 0), 1);
    acb_set_si(acb_mat_entry(matrix, 1, 1), 1);

    acb_t lambda_approx;
    acb_init(lambda_approx);
    acb_set_d(lambda_approx, 2.0);

    acb_mat_t vector_approx;
    acb_mat_init(vector_approx, kDimension, 1);
    acb_set_d(acb_mat_entry(vector_approx, 0, 0), 1.0);
    acb_set_d(acb_mat_entry(vector_approx, 1, 0), 1.0);

    acb_t lambda;
    acb_init(lambda);
    acb_mat_t vector;
    acb_mat_init(vector, kDimension, 1);

    acb_mat_eig_enclosure_rump(lambda, nullptr, vector, matrix, lambda_approx,
                               vector_approx, kPrecision);

    bool valid = acb_is_finite(lambda) && acb_mat_is_finite(vector) &&
                 contains_real_value(lambda, 2);
    for (slong i = 0; i < kDimension; ++i) {
        const acb_struct* entry = acb_mat_entry(vector, i, 0);
        valid = valid && arb_contains_zero(acb_imagref(entry)) &&
                arb_is_positive(acb_realref(entry));
    }

    // For this quotient, w_1 = (2, 2) and w_2 = (4, 4), so rho_2 = 1.
    const ulong w1[kDimension] = {2, 2};
    const ulong wd[kDimension] = {4, 4};
    arb_t numerator;
    arb_t denominator;
    arb_t vector_sum;
    arb_t lambda_power;
    arb_t rho;
    arb_t term;
    arb_t one;
    arb_init(numerator);
    arb_init(denominator);
    arb_init(vector_sum);
    arb_init(lambda_power);
    arb_init(rho);
    arb_init(term);
    arb_init(one);
    arb_zero(numerator);
    arb_zero(vector_sum);
    arb_one(one);
    for (slong i = 0; i < kDimension; ++i) {
        const arb_struct* component = acb_realref(acb_mat_entry(vector, i, 0));
        arb_mul_ui(term, component, w1[i] * wd[i], kPrecision);
        arb_add(numerator, numerator, term, kPrecision);
        arb_add(vector_sum, vector_sum, component, kPrecision);
    }
    arb_pow_ui(lambda_power, acb_realref(lambda), kDepth + 1, kPrecision);
    arb_mul(denominator, lambda_power, vector_sum, kPrecision);
    arb_div(rho, numerator, denominator, kPrecision);
    valid = valid && arb_contains(rho, one) && !arb_gt(rho, one) &&
            !arb_lt(rho, one);

    if (valid) {
        std::cout << "Certified Perron root enclosure: ";
        arb_printn(acb_realref(lambda), 30, ARB_STR_NO_RADIUS);
        std::cout << '\n' << "Certified eigenvector enclosure:\n";
        for (slong i = 0; i < kDimension; ++i) {
            std::cout << "  u_" << i << ": ";
            arb_printn(acb_realref(acb_mat_entry(vector, i, 0)), 30,
                       ARB_STR_NO_RADIUS);
            std::cout << '\n';
        }
        std::cout << "Certified rho_2 enclosure: ";
        arb_printn(rho, 30, 0);
        std::cout << '\n';
    } else {
        std::cerr << "Rump enclosure did not certify the expected real "
                     "positive Perron eigenpair.\n";
    }

    acb_mat_clear(matrix);
    acb_clear(lambda_approx);
    acb_mat_clear(vector_approx);
    acb_clear(lambda);
    acb_mat_clear(vector);
    arb_clear(numerator);
    arb_clear(denominator);
    arb_clear(vector_sum);
    arb_clear(lambda_power);
    arb_clear(rho);
    arb_clear(term);
    arb_clear(one);
    flint_cleanup_master();

    return valid ? 0 : 1;
}
