#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,fma")
// Fast asymptotic filter for Leontovich graphs using E_n^{(d)} trees.
// Compile: g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
// Usage:   geng -c 8 -q | ./leontovich_fast
//          genbg -c 7 8 -q | ./leontovich_fast   (bipartite m=15)
//
// v5: Dense 16x16 padded matrix with DAXPY loop order for AVX2 FMA.
//     Fixed loop bounds eliminate branch mispredictions entirely.

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

static constexpr int MAX_K = 201;
static constexpr int MAX_N = MAX_K + 1;  // homP needs index up to MAX_K
static constexpr int V_PAD = 16;

struct Graph {
    int m;
    alignas(32) double A[V_PAD][V_PAD];
};

static void parse_graph6(const char* g6, Graph& G) {
    if (strncmp(g6, ">>graph6<<", 10) == 0) g6 += 10;
    G.m = g6[0] - 63;
    memset(G.A, 0, sizeof(G.A));
    int k = 1, bit_pos = 5;
    for (int col = 1; col < G.m; col++) {
        for (int row = 0; row < col; row++) {
            int val = g6[k] - 63;
            if ((val >> bit_pos) & 1) {
                G.A[row][col] = 1.0;
                G.A[col][row] = 1.0;
            }
            if (--bit_pos < 0) {
                k++;
                bit_pos = 5;
            }
        }
    }
}

static bool check_leontovich(const Graph& G, const string& g6_str) {
    int m = G.m;

    // Precompute w[k] = A^k * 1 and homP[n] = sum_i w[n-1][i]
    alignas(32) double w[MAX_K][V_PAD];
    double homP[MAX_N];

    memset(w, 0, sizeof(w));
    memset(homP, 0, sizeof(homP));
    for (int i = 0; i < m; i++) w[0][i] = 1.0;
    homP[1] = m;

    for (int step = 1; step < MAX_K; step++) {
        alignas(32) double tmp[V_PAD] = {};
        for (int j = 0; j < V_PAD; j++) {
            double wj = w[step - 1][j];
#pragma GCC ivdep
            for (int i = 0; i < V_PAD; i++) {
                tmp[i] += G.A[j][i] * wj;
            }
        }
        double s = 0.0;
        for (int i = 0; i < V_PAD; i++) {
            w[step][i] = tmp[i];
            s += tmp[i];
        }
        if (step + 1 < MAX_N) homP[step + 1] = s;
    }

    // Precision check: find the step where values exceed 2^52
    // (beyond which double cannot represent consecutive integers)
    int precise_steps = MAX_K;
    for (int step = 0; step < MAX_K; step++) {
        for (int i = 0; i < m; i++) {
            if (w[step][i] > 4.5e15) {  // ~2^52
                precise_steps = step;
                goto precision_check_done;
            }
        }
    }
precision_check_done:

    // Check E_n^{(d)}: hom = sum_i w[stem][i] * w[1][i] * w[d][i]
    for (int d = 2; d <= 20; d++) {
        alignas(32) double b[V_PAD] = {};
#pragma GCC ivdep
        for (int i = 0; i < V_PAD; i++) {
            b[i] = w[1][i] * w[d][i];
        }

        int limit = 200 - d - 2;
        for (int stem = 0; stem <= limit; stem++) {
            double homE = 0.0;
#pragma GCC ivdep
            for (int i = 0; i < V_PAD; i++) {
                homE += w[stem][i] * b[i];
            }

            int n = stem + d + 2;
            if (homE < homP[n] * (1.0 - 1e-11)) {
                const char* type = (d > 2) ? "anomaly" : "leontovich";
#pragma omp critical
                {
                    cout << fixed << setprecision(1) << "{\"type\":\"" << type
                         << "\""
                         << ",\"g6\":\"" << g6_str << "\""
                         << ",\"m\":" << m << ",\"n\":" << n << ",\"d\":" << d
                         << ",\"homE\":" << homE << ",\"homP\":" << homP[n]
                         << ",\"precise_steps\":" << precise_steps << "}"
                         << endl;
                }
                return true;
            }
        }
    }
    return false;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    constexpr int BATCH = 250000;
    vector<string> batch;
    batch.reserve(BATCH);

    int64_t total_count = 0;
    int leontovich_count = 0;
    auto t0 = chrono::steady_clock::now();

    auto process_batch = [&]() {
        int bsz = (int)batch.size();
        int local_total_hits = 0;

#pragma omp parallel for reduction(+ : local_total_hits) schedule(dynamic, 1024)
        for (int idx = 0; idx < bsz; idx++) {
            Graph G;
            parse_graph6(batch[idx].c_str(), G);
            if (check_leontovich(G, batch[idx])) {
                local_total_hits++;
            }
        }

        leontovich_count += local_total_hits;
        total_count += bsz;
        batch.clear();

        auto now = chrono::steady_clock::now();
        double secs = chrono::duration<double>(now - t0).count();
        int rate = secs > 0 ? (int)(total_count / secs) : 0;
        cerr << "  [filter] " << total_count << " graphs | " << leontovich_count
             << " hits | " << rate << "/s | " << (int)secs << "s elapsed\r"
             << flush;
    };

    string line;
    while (cin >> line) {
        if (line.empty()) continue;
        batch.push_back(std::move(line));
        if ((int)batch.size() >= BATCH) process_batch();
    }
    if (!batch.empty()) process_batch();

    auto t1 = chrono::steady_clock::now();
    double total_secs = chrono::duration<double>(t1 - t0).count();
    int rate = total_count > 0 ? (int)(total_count / total_secs) : 0;
    cerr << "\nLeontovich filter: " << leontovich_count << " / " << total_count
         << " graphs flagged (n<=200, d<=20)"
         << " | " << (int)total_secs << "s"
         << " | " << rate << " graphs/s" << endl;

    cerr << "{\"event\":\"leontovich_filter_done\""
         << ",\"total\":" << total_count << ",\"hits\":" << leontovich_count
         << ",\"elapsed_s\":" << (int)total_secs << ",\"rate_per_s\":" << rate
         << "}" << endl;

    return 0;
}
