#include <flint/acb.h>
#include <flint/acb_mat.h>
#include <flint/arb.h>
#include <flint/flint.h>

#include <array>
#include <iostream>
#include <string>

namespace {

constexpr slong kDimension = 2;
constexpr ulong kDepth = 2;
constexpr std::array<slong, 4> kPrecisions = {64, 128, 256, 512};

enum class SignStatus { kBelow, kAbove, kUnresolved, kInvalid };

struct AttemptResult {
    SignStatus status = SignStatus::kInvalid;
    std::string rho_interval = "indeterminate";
};

AttemptResult certify_at_precision(slong precision) {
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
                               vector_approx, precision);

    bool valid = acb_is_finite(lambda) && acb_mat_is_finite(vector) &&
                 arb_contains_si(acb_realref(lambda), 2) &&
                 arb_contains_zero(acb_imagref(lambda));
    for (slong i = 0; i < kDimension; ++i) {
        const acb_struct* entry = acb_mat_entry(vector, i, 0);
        valid = valid && arb_contains_zero(acb_imagref(entry)) &&
                arb_is_positive(acb_realref(entry));
    }

    // For this quotient, w_1 = (2, 2) and w_2 = (4, 4), so rho_2 = 1.
    constexpr std::array<ulong, kDimension> w1 = {2, 2};
    constexpr std::array<ulong, kDimension> wd = {4, 4};
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
        arb_mul_ui(term, component, w1[i] * wd[i], precision);
        arb_add(numerator, numerator, term, precision);
        arb_add(vector_sum, vector_sum, component, precision);
    }
    arb_pow_ui(lambda_power, acb_realref(lambda), kDepth + 1, precision);
    arb_mul(denominator, lambda_power, vector_sum, precision);
    arb_div(rho, numerator, denominator, precision);

    SignStatus status = SignStatus::kInvalid;
    if (valid && arb_is_finite(rho)) {
        if (arb_gt(rho, one)) {
            status = SignStatus::kAbove;
        } else if (arb_lt(rho, one)) {
            status = SignStatus::kBelow;
        } else {
            status = SignStatus::kUnresolved;
        }
    }
    char* interval = arb_get_str(rho, 30, 0);
    std::string rho_interval = interval == nullptr ? "indeterminate" : interval;
    flint_free(interval);

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
    return {status, rho_interval};
}

const char* status_name(SignStatus status) {
    switch (status) {
        case SignStatus::kBelow:
            return "certified_below_one";
        case SignStatus::kAbove:
            return "certified_above_one";
        case SignStatus::kUnresolved:
            return "unresolved";
        case SignStatus::kInvalid:
            return "invalid_enclosure";
    }
    return "invalid_enclosure";
}

}  // namespace

int main() {
    AttemptResult result{SignStatus::kInvalid, "indeterminate"};
    slong final_precision = 0;
    for (slong precision : kPrecisions) {
        result = certify_at_precision(precision);
        final_precision = precision;
        if (result.status == SignStatus::kAbove ||
            result.status == SignStatus::kBelow) {
            break;
        }
    }

    std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
              << "\"status\":\"" << status_name(result.status) << "\","
              << "\"depth\":" << kDepth << ','
              << "\"precision_bits\":" << final_precision << ','
              << "\"rho_interval\":\"" << result.rho_interval << "\"}\n";
    flint_cleanup_master();

    // This hardcoded boundary case is a fail-closed self-test: choosing either
    // strict sign would be an error, while unresolved at 512 bits is expected.
    return result.status == SignStatus::kUnresolved && final_precision == 512
               ? 0
               : 1;
}
