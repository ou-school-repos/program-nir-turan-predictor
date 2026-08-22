#include <flint/acb.h>
#include <flint/acb_mat.h>
#include <flint/arb.h>
#include <flint/flint.h>
#include <flint/fmpz_mat.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::array<slong, 4> kPrecisions = {64, 128, 256, 512};

enum class SignStatus { kBelow, kAbove, kUnresolved, kInvalid };

struct AttemptResult {
    SignStatus status = SignStatus::kInvalid;
    std::string rho_interval = "indeterminate";
};

struct Graph {
    std::vector<std::vector<int>> adjacency;
};

struct PerronApproximation {
    bool converged = false;
    double eigenvalue = 0.0;
    double residual = 0.0;
    std::vector<double> eigenvector;
};

Graph parse_graph6(std::string encoded) {
    constexpr std::string_view header = ">>graph6<<";
    if (encoded.rfind(header, 0) == 0) {
        encoded.erase(0, header.size());
    }
    if (encoded.empty() || static_cast<unsigned char>(encoded[0]) < 64 ||
        static_cast<unsigned char>(encoded[0]) > 125) {
        throw std::invalid_argument(
            "only short-form graph6 with 1..62 vertices is supported");
    }
    const size_t dimension = static_cast<unsigned char>(encoded[0]) - 63;
    const size_t edge_bits = dimension * (dimension - 1) / 2;
    const size_t required_size = 1 + (edge_bits + 5) / 6;
    if (encoded.size() != required_size) {
        throw std::invalid_argument(
            "graph6 payload length does not match its order");
    }
    const bool invalid_character =
        std::any_of(encoded.begin(), encoded.end(), [](char value) {
            const auto byte = static_cast<unsigned char>(value);
            return byte < 63 || byte > 126;
        });
    if (invalid_character) {
        throw std::invalid_argument("graph6 contains an invalid character");
    }

    std::vector<std::vector<int>> adjacency(dimension,
                                            std::vector<int>(dimension, 0));
    size_t character = 1;
    int bit = 5;
    for (size_t column = 1; column < dimension; ++column) {
        for (size_t row = 0; row < column; ++row) {
            const int value =
                static_cast<unsigned char>(encoded[character]) - 63;
            if ((value >> bit) & 1) {
                adjacency[row][column] = 1;
                adjacency[column][row] = 1;
            }
            if (--bit < 0) {
                ++character;
                bit = 5;
            }
        }
    }
    return {std::move(adjacency)};
}

bool is_connected(const Graph& graph) {
    std::vector<bool> seen(graph.adjacency.size(), false);
    std::vector<size_t> stack = {0};
    seen[0] = true;
    while (!stack.empty()) {
        const size_t vertex = stack.back();
        stack.pop_back();
        for (size_t neighbor = 0; neighbor < graph.adjacency.size();
             ++neighbor) {
            if (graph.adjacency[vertex][neighbor] && !seen[neighbor]) {
                seen[neighbor] = true;
                stack.push_back(neighbor);
            }
        }
    }
    return std::all_of(seen.begin(), seen.end(),
                       [](bool reached) { return reached; });
}

bool is_bipartite(const Graph& graph) {
    std::vector<int> color(graph.adjacency.size(), -1);
    color[0] = 0;
    std::vector<size_t> stack = {0};
    while (!stack.empty()) {
        const size_t vertex = stack.back();
        stack.pop_back();
        for (size_t neighbor = 0; neighbor < graph.adjacency.size();
             ++neighbor) {
            if (!graph.adjacency[vertex][neighbor]) continue;
            if (color[neighbor] < 0) {
                color[neighbor] = 1 - color[vertex];
                stack.push_back(neighbor);
            } else if (color[neighbor] == color[vertex]) {
                return false;
            }
        }
    }
    return true;
}

PerronApproximation approximate_perron(const Graph& graph) {
    constexpr int iteration_cap = 256;
    constexpr double residual_tolerance = 1e-13;
    const size_t dimension = graph.adjacency.size();
    std::vector<double> vector(dimension, 1.0 / std::sqrt(dimension));
    std::vector<double> product(dimension, 0.0);
    PerronApproximation result;
    for (int iteration = 0; iteration < iteration_cap; ++iteration) {
        std::fill(product.begin(), product.end(), 0.0);
        for (size_t row = 0; row < dimension; ++row) {
            for (size_t column = 0; column < dimension; ++column) {
                product[row] += graph.adjacency[row][column] * vector[column];
            }
        }
        double eigenvalue = 0.0;
        for (size_t i = 0; i < dimension; ++i) {
            eigenvalue += vector[i] * product[i];
        }
        double residual_squared = 0.0;
        double norm_squared = 0.0;
        for (size_t i = 0; i < dimension; ++i) {
            const double error = product[i] - eigenvalue * vector[i];
            residual_squared += error * error;
            norm_squared += product[i] * product[i];
        }
        result = {std::sqrt(residual_squared) < residual_tolerance, eigenvalue,
                  std::sqrt(residual_squared), vector};
        if (result.converged || !(norm_squared > 0.0) ||
            !std::isfinite(norm_squared)) {
            return result;
        }
        const double inverse_norm = 1.0 / std::sqrt(norm_squared);
        for (size_t i = 0; i < dimension; ++i) {
            vector[i] = product[i] * inverse_norm;
        }
    }
    return result;
}

AttemptResult certify_at_precision(slong precision, const Graph& graph,
                                   const PerronApproximation& approximation,
                                   ulong depth) {
    const slong dimension = static_cast<slong>(graph.adjacency.size());
    fmpz_mat_t exact_matrix;
    fmpz_mat_t exact_power;
    fmpz_mat_t ones;
    fmpz_mat_t w1;
    fmpz_mat_t wd;
    fmpz_mat_init(exact_matrix, dimension, dimension);
    fmpz_mat_init(exact_power, dimension, dimension);
    fmpz_mat_init(ones, dimension, 1);
    fmpz_mat_init(w1, dimension, 1);
    fmpz_mat_init(wd, dimension, 1);
    for (slong row = 0; row < dimension; ++row) {
        fmpz_one(fmpz_mat_entry(ones, row, 0));
        for (slong column = 0; column < dimension; ++column) {
            fmpz_set_si(fmpz_mat_entry(exact_matrix, row, column),
                        graph.adjacency[row][column]);
        }
    }
    fmpz_mat_mul(w1, exact_matrix, ones);
    fmpz_mat_pow(exact_power, exact_matrix, depth);
    fmpz_mat_mul(wd, exact_power, ones);

    acb_mat_t matrix;
    acb_mat_init(matrix, dimension, dimension);
    acb_mat_set_fmpz_mat(matrix, exact_matrix);

    acb_t lambda_approx;
    acb_init(lambda_approx);
    acb_set_d(lambda_approx, approximation.eigenvalue);
    acb_mat_t vector_approx;
    acb_mat_init(vector_approx, dimension, 1);
    for (slong row = 0; row < dimension; ++row) {
        acb_set_d(acb_mat_entry(vector_approx, row, 0),
                  approximation.eigenvector[row]);
    }

    acb_t lambda;
    acb_init(lambda);
    acb_mat_t vector;
    acb_mat_init(vector, dimension, 1);
    acb_mat_eig_enclosure_rump(lambda, nullptr, vector, matrix, lambda_approx,
                               vector_approx, precision);

    bool valid = acb_is_finite(lambda) && acb_mat_is_finite(vector) &&
                 arb_is_positive(acb_realref(lambda)) &&
                 arb_contains_zero(acb_imagref(lambda));
    for (slong i = 0; i < dimension; ++i) {
        const acb_struct* entry = acb_mat_entry(vector, i, 0);
        valid = valid && arb_contains_zero(acb_imagref(entry)) &&
                arb_is_positive(acb_realref(entry));
    }

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
    fmpz_t coefficient;
    fmpz_init(coefficient);
    for (slong i = 0; i < dimension; ++i) {
        const arb_struct* component = acb_realref(acb_mat_entry(vector, i, 0));
        fmpz_mul(coefficient, fmpz_mat_entry(w1, i, 0),
                 fmpz_mat_entry(wd, i, 0));
        arb_mul_fmpz(term, component, coefficient, precision);
        arb_add(numerator, numerator, term, precision);
        arb_add(vector_sum, vector_sum, component, precision);
    }
    arb_pow_ui(lambda_power, acb_realref(lambda), depth + 1, precision);
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
    fmpz_mat_clear(exact_matrix);
    fmpz_mat_clear(exact_power);
    fmpz_mat_clear(ones);
    fmpz_mat_clear(w1);
    fmpz_mat_clear(wd);
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
    fmpz_clear(coefficient);
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

int main(int argc, char** argv) {
    if (argc > 1) {
        std::string graph6;
        int depth = 2;
        try {
            for (int i = 1; i < argc; ++i) {
                const std::string argument = argv[i];
                if (argument == "--g6" && i + 1 < argc) {
                    graph6 = argv[++i];
                } else if (argument == "--depth" && i + 1 < argc) {
                    depth = std::stoi(argv[++i]);
                } else {
                    std::cerr << "Usage: " << argv[0]
                              << " [--g6 GRAPH6 --depth D]\n";
                    return 2;
                }
            }
        } catch (const std::exception&) {
            std::cerr << "invalid --depth value\n";
            return 2;
        }
        if (graph6.empty() || depth < 1) {
            std::cerr << "graph6 input and a positive depth are required\n";
            return 2;
        }
        try {
            const Graph graph = parse_graph6(graph6);
            if (!is_connected(graph)) {
                std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
                          << "\"status\":\"invalid_disconnected\","
                          << "\"depth\":" << depth << "}\n";
                return 1;
            }
            if (is_bipartite(graph)) {
                std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
                          << "\"status\":\"deferred_bipartite_parity\","
                          << "\"vertices\":" << graph.adjacency.size() << ','
                          << "\"depth\":" << depth << "}\n";
                return 0;
            }
            const PerronApproximation approximation = approximate_perron(graph);
            const bool positive_initializer =
                std::all_of(approximation.eigenvector.begin(),
                            approximation.eigenvector.end(),
                            [](double component) { return component > 0.0; });
            const bool initializer_ready =
                approximation.converged && positive_initializer;
            std::cout.precision(17);
            if (!initializer_ready) {
                std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
                          << "\"status\":\"unresolved_power_iteration\","
                          << "\"vertices\":" << graph.adjacency.size() << ','
                          << "\"depth\":" << depth << ','
                          << "\"lambda_approx\":" << approximation.eigenvalue
                          << ",\"residual\":" << approximation.residual
                          << "}\n";
                return 1;
            }

            AttemptResult result;
            slong final_precision = 0;
            for (slong precision : kPrecisions) {
                result = certify_at_precision(precision, graph, approximation,
                                              static_cast<ulong>(depth));
                final_precision = precision;
                if (result.status == SignStatus::kAbove ||
                    result.status == SignStatus::kBelow) {
                    break;
                }
            }
            std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
                      << "\"status\":\"" << status_name(result.status) << "\","
                      << "\"vertices\":" << graph.adjacency.size() << ','
                      << "\"depth\":" << depth << ','
                      << "\"precision_bits\":" << final_precision << ','
                      << "\"lambda_approx\":" << approximation.eigenvalue << ','
                      << "\"residual\":" << approximation.residual << ','
                      << "\"rho_interval\":\"" << result.rho_interval
                      << "\"}\n";
            return result.status == SignStatus::kInvalid ? 1 : 0;
        } catch (const std::exception& error) {
            std::cerr << "Invalid input: " << error.what() << '\n';
            return 2;
        }
    }

    const Graph graph{{{0, 2}, {1, 1}}};
    const PerronApproximation approximation{true, 2.0, 0.0, {1.0, 1.0}};
    constexpr ulong depth = 2;
    AttemptResult result;
    slong final_precision = 0;
    for (slong precision : kPrecisions) {
        result = certify_at_precision(precision, graph, approximation, depth);
        final_precision = precision;
        if (result.status == SignStatus::kAbove ||
            result.status == SignStatus::kBelow) {
            break;
        }
    }

    std::cout << "{\"schema\":\"arb-rho-certificate-v1\","
              << "\"status\":\"" << status_name(result.status) << "\","
              << "\"depth\":" << depth << ','
              << "\"precision_bits\":" << final_precision << ','
              << "\"rho_interval\":\"" << result.rho_interval << "\"}\n";
    flint_cleanup_master();

    // This hardcoded boundary case is a fail-closed self-test: choosing either
    // strict sign would be an error, while unresolved at 512 bits is expected.
    return result.status == SignStatus::kUnresolved && final_precision == 512
               ? 0
               : 1;
}
