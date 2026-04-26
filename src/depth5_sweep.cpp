#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,fma")
// Depth-5 sweep: Pareto frontier of smallest Leontovich graphs
// among all spherically symmetric trees T(d1,...,dk) for k ≤ 5.
//
// Uses the (k+1)×(k+1) similarity matrix instead of the full |V|×|V|
// adjacency matrix -- O(k²) per evaluation instead of O(|V|²).
//
// Compile: g++ -O3 -march=native -std=c++17 -o depth5_sweep
//          src/depth5_sweep.cpp
// Usage:   ./depth5_sweep [--max-vertices 400] [--max-degree 20]
//                         [--max-depth 5]

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static constexpr int MAX_DIM = 7;  // max orbits = max_depth + 1
static constexpr int MAX_N = 301;  // max path length to check

struct SimilarityMatrix {
    int dim;
    double M[MAX_DIM][MAX_DIM];
    int a[MAX_DIM];  // orbit sizes
    int total_v;

    void build(const std::vector<int>& degrees) {
        dim = (int)degrees.size() + 1;
        memset(M, 0, sizeof(M));
        for (int i = 0; i < (int)degrees.size(); i++) {
            M[i][i + 1] = degrees[i];
            M[i + 1][i] = 1;
        }
        a[0] = 1;
        for (int i = 0; i < (int)degrees.size(); i++)
            a[i + 1] = a[i] * degrees[i];
        total_v = 0;
        for (int i = 0; i < dim; i++) total_v += a[i];
    }
};

struct LeoResult {
    bool found;
    int first_n;
    int first_d;
    double ratio;
};

static LeoResult check_leontovich(const SimilarityMatrix& S) {
    int dim = S.dim;

    // v[step][orbit] = (M^step · 1)[orbit]
    static double v[MAX_N][MAX_DIM];
    static double homP[MAX_N + 1];

    memset(v, 0, sizeof(double) * MAX_N * MAX_DIM);
    memset(homP, 0, sizeof(homP));

    for (int i = 0; i < dim; i++) v[0][i] = 1.0;
    homP[1] = S.total_v;

    for (int step = 1; step < MAX_N; step++) {
        for (int i = 0; i < dim; i++) {
            double s = 0;
            for (int j = 0; j < dim; j++) s += S.M[i][j] * v[step - 1][j];
            v[step][i] = s;
        }
        double h = 0;
        for (int i = 0; i < dim; i++) h += S.a[i] * v[step][i];
        homP[step + 1] = h;
    }

    // Check crossovers
    for (int d = 2; d < 21; d++) {
        double b[MAX_DIM];
        for (int i = 0; i < dim; i++) b[i] = v[1][i] * v[d][i];

        for (int stem = 0; stem < MAX_N - d - 2; stem++) {
            int n = stem + d + 2;
            if (n >= MAX_N) break;
            double homE = 0;
            for (int i = 0; i < dim; i++) homE += S.a[i] * v[stem][i] * b[i];
            if (homP[n] > 0 && homE / homP[n] < 1.0 - 1e-11) {
                return {true, n, d, homE / homP[n]};
            }
        }
    }
    return {false, 0, 0, 0};
}

struct FrontierEntry {
    int total_v;
    std::vector<int> degrees;
    int d;
    double ratio;
};

static int max_vertices = 400;
static int max_degree = 20;
static int max_depth = 5;
static long long total_checked = 0;
static long long total_leo = 0;
static std::map<int, FrontierEntry> frontier;

static void sweep(std::vector<int>& current, int remaining, int orbit_product,
                  int current_v) {
    if (remaining == 0) {
        total_checked++;

        SimilarityMatrix S;
        S.build(current);

        auto result = check_leontovich(S);
        if (result.found) {
            total_leo++;
            int n = result.first_n;
            if (frontier.find(n) == frontier.end() ||
                S.total_v < frontier[n].total_v) {
                frontier[n] = {S.total_v, current, result.first_d,
                               result.ratio};
                printf("  NEW BEST n>=%d: T(", n);
                for (int i = 0; i < (int)current.size(); i++) {
                    if (i) printf(",");
                    printf("%d", current[i]);
                }
                printf(") |V|=%d d=%d r=%.8f\n", S.total_v, result.first_d,
                       result.ratio);
                fflush(stdout);
            }
        }

        if (total_checked % 100000 == 0) {
            fprintf(stderr, "  ... checked %lld sequences, found %lld Leo\n",
                    total_checked, total_leo);
        }
        return;
    }

    for (int d = 1; d <= max_degree; d++) {
        int new_v = current_v + orbit_product * d;
        if (new_v > max_vertices) break;
        current.push_back(d);
        sweep(current, remaining - 1, orbit_product * d, new_v);
        current.pop_back();
    }
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--max-vertices" && i + 1 < argc)
            max_vertices = std::stoi(argv[++i]);
        else if (arg == "--max-degree" && i + 1 < argc)
            max_degree = std::stoi(argv[++i]);
        else if (arg == "--max-depth" && i + 1 < argc)
            max_depth = std::stoi(argv[++i]);
        else if (arg == "-h" || arg == "--help") {
            printf(
                "Usage: %s [--max-vertices N] [--max-degree N] [--max-depth "
                "N]\n",
                argv[0]);
            return 0;
        }
    }

    printf("=== Depth-%d Leontovich Sweep (C++) ===\n", max_depth);
    printf("Max vertices: %d, Max degree: %d\n\n", max_vertices, max_degree);

    for (int depth = 1; depth <= max_depth; depth++) {
        std::vector<int> seq;
        sweep(seq, depth, 1, 1);
    }

    printf("\n============================================================\n");
    printf("Checked %lld branching sequences, found %lld Leontovich\n",
           total_checked, total_leo);
    printf("\n============================================================\n");
    printf("PARETO FRONTIER: Smallest Leontovich graph per threshold n\n");
    printf("============================================================\n");
    printf("%4s  %6s  %-30s  %3s  %12s\n", "n", "|V|", "Structure", "d",
           "ratio");
    printf(
        "--------------------------------------------------------------------"
        "\n");

    int abs_min_n = -1, abs_min_v = 1 << 30;
    for (auto& [n, e] : frontier) {
        printf("%4d  %6d  T(", n, e.total_v);
        for (int i = 0; i < (int)e.degrees.size(); i++) {
            if (i) printf(",");
            printf("%d", e.degrees[i]);
        }
        printf(")%*s  %3d  %12.8f\n", (int)(27 - 2 * e.degrees.size()), "", e.d,
               e.ratio);
        if (e.total_v < abs_min_v) {
            abs_min_v = e.total_v;
            abs_min_n = n;
        }
    }

    if (abs_min_n >= 0) {
        const auto& e = frontier[abs_min_n];
        printf("\nAbsolute minimum: T(");
        for (int i = 0; i < (int)e.degrees.size(); i++) {
            if (i) printf(",");
            printf("%d", e.degrees[i]);
        }
        printf(") with |V|=%d at n>=%d\n", e.total_v, abs_min_n);
    }

    return 0;
}
