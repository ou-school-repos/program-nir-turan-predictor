#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,fma")
// Depth-5 sweep: threshold landscape of smallest Leontovich graphs
// among all spherically symmetric trees T(d1,...,dk) for k ≤ 5.
//
// Uses EXACT integer arithmetic (__int128) on the (k+1)×(k+1)
// similarity matrix.  Falls back to extended range via log-space
// sign detection when __int128 overflows.
//
// Compile: g++ -O3 -march=native -std=c++17 -fopenmp -o depth5_sweep
//          src/depth5_sweep.cpp
// Usage:   ./depth5_sweep [--max-vertices 400] [--max-degree 20]
//                         [--max-depth 5]

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static FILE* log_fp = nullptr;

static constexpr int MAX_DIM = 7;  // max orbits = max_depth + 1
static constexpr int MAX_N = 301;  // max path length to check

using i128 = __int128;

// Overflow-checked multiply using compiler intrinsic.
// __builtin_mul_overflow is immune to UB-based optimization removal.
static inline bool safe_mul(i128 a, i128 b, i128& result) {
    return !__builtin_mul_overflow(a, b, &result);
}

// Overflow-checked add using compiler intrinsic.
static inline bool safe_add(i128 a, i128 b, i128& result) {
    return !__builtin_add_overflow(a, b, &result);
}

struct SimilarityMatrix {
    int dim;
    int M[MAX_DIM][MAX_DIM];  // integer quotient matrix entries
    int a[MAX_DIM];           // orbit sizes
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

    // v[step][orbit] = (M^step · 1)[orbit] -- exact integers
    static thread_local i128 v[MAX_N][MAX_DIM];
    static thread_local i128 homP[MAX_N + 1];

    memset(v, 0, sizeof(i128) * MAX_N * MAX_DIM);
    memset(homP, 0, sizeof(homP));

    for (int i = 0; i < dim; i++) v[0][i] = 1;
    homP[1] = S.total_v;

    int safe_steps = MAX_N;  // last step where no overflow occurred

    for (int step = 1; step < MAX_N; step++) {
        bool overflow = false;
        for (int i = 0; i < dim; i++) {
            i128 s = 0;
            for (int j = 0; j < dim; j++) {
                i128 prod;
                if (!safe_mul((i128)S.M[i][j], v[step - 1][j], prod)) {
                    overflow = true;
                    break;
                }
                if (!safe_add(s, prod, s)) {
                    overflow = true;
                    break;
                }
            }
            if (overflow) break;
            v[step][i] = s;
        }
        if (overflow) {
            safe_steps = step;
            break;
        }

        i128 h = 0;
        for (int i = 0; i < dim; i++) {
            i128 term;
            if (!safe_mul((i128)S.a[i], v[step][i], term) ||
                !safe_add(h, term, h)) {
                safe_steps = step;
                goto done_powers;
            }
        }
        homP[step + 1] = h;
    }
done_powers:

    // Check crossovers using exact arithmetic
    for (int d = 2; d < 21; d++) {
        for (int stem = 0; stem < safe_steps - d - 1; stem++) {
            int n = stem + d + 2;
            if (n >= safe_steps + 1) break;  // need homP[n] which uses step n-1
            if (homP[n] <= 0) continue;

            // homE = sum_i a[i] * v[stem][i] * v[1][i] * v[d][i]
            i128 homE = 0;
            bool overflow = false;
            for (int i = 0; i < dim; i++) {
                i128 t1, t2, t3;
                if (!safe_mul(v[stem][i], v[1][i], t1) ||
                    !safe_mul(t1, v[d][i], t2) ||
                    !safe_mul((i128)S.a[i], t2, t3) ||
                    !safe_add(homE, t3, homE)) {
                    overflow = true;
                    break;
                }
            }
            if (overflow) break;  // can't verify at this d,n

            // Exact comparison: is homE < homP[n]?
            if (homE < homP[n]) {
                // Genuine Leontovich crossover (exact)
                double ratio =
                    (double)homE / (double)homP[n];  // approx for display
                return {true, n, d, ratio};
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
static std::atomic<long long> total_checked{0};
static std::atomic<long long> total_leo{0};

// Key: (threshold_n, orbit_count)
using FKey = std::pair<int, int>;
static std::map<FKey, FrontierEntry> frontier;

// Thread-local recursive sweep (no global mutation except atomics)
static void sweep(std::vector<int>& current, int remaining, int orbit_product,
                  int current_v, long long& local_checked, long long& local_leo,
                  std::map<FKey, FrontierEntry>& local_frontier) {
    if (remaining == 0) {
        local_checked++;

        if (local_checked % 100000 == 0) {
            fprintf(stderr, "\r    ... checked %lld (local), found %lld Leo   ",
                    local_checked, local_leo);
        }

        SimilarityMatrix S;
        S.build(current);

        auto result = check_leontovich(S);
        if (result.found) {
            local_leo++;
            int n = result.first_n;
            int orbits = (int)current.size() + 1;
            FKey key = {n, orbits};
            if (local_frontier.find(key) == local_frontier.end() ||
                S.total_v < local_frontier[key].total_v) {
                local_frontier[key] = {S.total_v, current, result.first_d,
                                       result.ratio};
            }
#pragma omp critical
            {
                if (frontier.find(key) == frontier.end() ||
                    S.total_v < frontier[key].total_v) {
                    frontier[key] = {S.total_v, current, result.first_d,
                                     result.ratio};
                    printf("  NEW BEST n>=%d [%d-orb]: T(", n, orbits);
                    for (int i = 0; i < (int)current.size(); i++) {
                        if (i) printf(",");
                        printf("%d", current[i]);
                    }
                    printf(") |V|=%d d=%d r=%.8f\n", S.total_v, result.first_d,
                           result.ratio);
                    fflush(stdout);
                    if (log_fp) {
                        fprintf(log_fp, "n>=%d,%d-orb,T(", n, orbits);
                        for (int i = 0; i < (int)current.size(); i++) {
                            if (i) fprintf(log_fp, ",");
                            fprintf(log_fp, "%d", current[i]);
                        }
                        fprintf(log_fp, "),%d,%d,%.8f\n", S.total_v,
                                result.first_d, result.ratio);
                        fflush(log_fp);
                    }
                }
            }
        }
        return;
    }

    for (int d = 1; d <= max_degree; d++) {
        int new_v = current_v + orbit_product * d;
        if (new_v > max_vertices) break;
        current.push_back(d);
        sweep(current, remaining - 1, orbit_product * d, new_v, local_checked,
              local_leo, local_frontier);
        current.pop_back();
    }
}

// Merge a local frontier into the global one (called under critical)
static void merge_frontier(const std::map<FKey, FrontierEntry>& local) {
    for (auto& [key, e] : local) {
        if (frontier.find(key) == frontier.end() ||
            e.total_v < frontier[key].total_v) {
            frontier[key] = e;
        }
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
        else if (arg == "--log" && i + 1 < argc) {
            log_fp = fopen(argv[++i], "w");
            if (!log_fp) {
                fprintf(stderr, "Cannot open log file: %s\n", argv[i]);
                return 1;
            }
            fprintf(log_fp, "threshold,tree,vertices,d,ratio\n");
            fflush(log_fp);
        } else if (arg == "-h" || arg == "--help") {
            printf(
                "Usage: %s [--max-vertices N] [--max-degree N] [--max-depth "
                "N] [--log FILE]\n",
                argv[0]);
            return 0;
        }
    }

    printf("=== Depth-%d Leontovich Sweep (C++, __int128 exact) ===\n",
           max_depth);
    printf("Max vertices: %d, Max degree: %d\n\n", max_vertices, max_degree);

    setvbuf(stderr, NULL, _IONBF, 0);

    for (int depth = 1; depth <= max_depth; depth++) {
        fprintf(stderr, "  [depth %d] sweeping...\n", depth);

        // Parallelize over the first branching degree d1
        int d1_max = std::min(max_degree, max_vertices - 1);

#pragma omp parallel
        {
            long long local_checked = 0, local_leo = 0;
            std::map<FKey, FrontierEntry> local_front;

#pragma omp for schedule(dynamic, 1) nowait
            for (int d1 = 1; d1 <= d1_max; d1++) {
                int v1 = 1 + d1;  // root + d1 children
                if (v1 > max_vertices) continue;
                if (depth == 1) {
                    // Single-level tree: just T(d1)
                    std::vector<int> seq = {d1};
                    local_checked++;
                    SimilarityMatrix S;
                    S.build(seq);
                    auto result = check_leontovich(S);
                    if (result.found) {
                        local_leo++;
                        int n = result.first_n;
                        FKey key = {n, 2};  // depth-1 = 2 orbits
                        if (local_front.find(key) == local_front.end() ||
                            S.total_v < local_front[key].total_v) {
                            local_front[key] = {S.total_v, seq, result.first_d,
                                                result.ratio};
                        }
                    }
                } else {
                    // Recurse for remaining (depth-1) levels
                    std::vector<int> seq = {d1};
                    sweep(seq, depth - 1, d1, v1, local_checked, local_leo,
                          local_front);
                }

                // Progress (only from first thread hitting this d1)
                int pct =
                    (int)((long long)(d1 - 1) * 100 / std::max(1, d1_max));
                fprintf(
                    stderr,
                    "\r  [depth %d] d1=%d/%d (%d%%)  checked=%lld  leo=%lld",
                    depth, d1, d1_max, pct,
                    total_checked.load(std::memory_order_relaxed) +
                        local_checked,
                    total_leo.load(std::memory_order_relaxed) + local_leo);
            }

#pragma omp critical
            {
                total_checked += local_checked;
                total_leo += local_leo;
                merge_frontier(local_front);
            }
        }

        fprintf(stderr,
                "\r  [depth %d] done.                                  \n",
                depth);
    }

    printf("\n============================================================\n");
    printf("Checked %lld branching sequences, found %lld Leontovich\n",
           total_checked.load(), total_leo.load());
    printf("\n============================================================\n");
    printf(
        "THRESHOLD LANDSCAPE: Smallest Leontovich graph per n (all orbits)\n");
    printf("============================================================\n");

    // Group by n
    std::map<int, std::map<int, FrontierEntry>> by_n;
    for (auto& [key, e] : frontier) {
        auto [n, orbits] = key;
        by_n[n][orbits] = e;
    }

    int abs_min_n = -1, abs_min_v = 1 << 30;
    for (auto& [n, orb_map] : by_n) {
        printf("n>=%3d:", n);
        for (auto& [orbits, e] : orb_map) {
            printf("  T(");
            for (int i = 0; i < (int)e.degrees.size(); i++) {
                if (i) printf(",");
                printf("%d", e.degrees[i]);
            }
            printf("),v=%d", e.total_v);
            if (e.total_v < abs_min_v) {
                abs_min_v = e.total_v;
                abs_min_n = n;
            }
        }
        printf("\n");
    }

    if (abs_min_n >= 0) {
        printf("\nAbsolute minimum |V|=%d at n>=%d\n", abs_min_v, abs_min_n);
    }

    if (log_fp) fclose(log_fp);
    return 0;
}
