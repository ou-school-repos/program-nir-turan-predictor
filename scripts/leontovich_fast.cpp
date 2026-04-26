// Fast asymptotic filter for Leontovich graphs using E_n^{(d)} trees.
// Compile: g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
// Usage:   geng -c 8 -q | ./leontovich_fast
//          genbg -c 7 8 -q | ./leontovich_fast   (bipartite m=15)
//
// v4: Bitboard adjacency (132 bytes vs 4KB), SWAR bit-scans for matvec,
//     precomputed w_k = A^k * 1 for O(N*m) per graph.

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static constexpr int MAX_V = 32;
static constexpr int MAX_K = 201;

struct Graph {
    int m;
    uint32_t mask[MAX_V];  // bitboard adjacency: 132 bytes total
};

static void parse_graph6(const char* g6, Graph& G) {
    if (strncmp(g6, ">>graph6<<", 10) == 0) g6 += 10;
    G.m = g6[0] - 63;
    memset(G.mask, 0, sizeof(G.mask));
    int k = 1, bit_pos = 5;
    for (int col = 1; col < G.m; col++) {
        for (int row = 0; row < col; row++) {
            int val = g6[k] - 63;
            if ((val >> bit_pos) & 1) {
                G.mask[row] |= (1U << col);
                G.mask[col] |= (1U << row);
            }
            if (--bit_pos < 0) {
                k++;
                bit_pos = 5;
            }
        }
    }
}

// SWAR matvec: y = A * x using hardware bit-scans
static inline void matvec_adj(const Graph& G, const double* __restrict x,
                              double* __restrict y) {
    for (int i = 0; i < G.m; i++) {
        double s = 0.0;
        uint32_t bits = G.mask[i];
        while (bits) {
            int j = __builtin_ctz(bits);  // TZCNT: find lowest set bit
            s += x[j];
            bits &= (bits - 1);  // BLSR: clear lowest set bit
        }
        y[i] = s;
    }
}

static bool check_leontovich(const Graph& G, const string& g6_str) {
    int m = G.m;

    // Precompute w[k][i] = (A^k * 1)_i for k = 0..200
    double w[MAX_K][MAX_V];
    for (int i = 0; i < m; i++) w[0][i] = 1.0;
    for (int k = 1; k < MAX_K; k++) {
        matvec_adj(G, w[k - 1], w[k]);
    }

    // hom(P_n, H) = sum_i w[n-1][i]
    double homP[MAX_K];
    for (int n = 1; n < MAX_K; n++) {
        double s = 0.0;
        for (int i = 0; i < m; i++) s += w[n - 1][i];
        homP[n] = s;
    }

    // hom(E_n^{(d)}, H) = sum_i w[n-d-2][i] * w[1][i] * w[d][i]
    for (int d = 2; d <= 20; d++) {
        double b[MAX_V];
        for (int i = 0; i < m; i++) {
            b[i] = w[1][i] * w[d][i];
        }

        for (int n = d + 3; n <= 200; n++) {
            int stem = n - d - 2;
            if (stem < 0) continue;

            double homE = 0.0;
            for (int i = 0; i < m; i++) {
                homE += w[stem][i] * b[i];
            }

            if (homE < homP[n] * (1.0 - 1e-11)) {
                if (d > 2) {
                    cout << "ANOMALY " << g6_str << " |V|=" << m << " n=" << n
                         << " d=" << d << " (d>2 required to beat path!)"
                         << " hom(E)=" << homE << " < hom(P)=" << homP[n]
                         << endl;
                } else {
                    cout << "LEONTOVICH " << g6_str << " |V|=" << m
                         << " n=" << n << " d=" << d << " hom(E)=" << homE
                         << " < hom(P)=" << homP[n] << endl;
                }
                return true;
            }
        }
    }
    return false;
}

int main() {
    string line;
    int leontovich_count = 0;
    int total_count = 0;
    auto t0 = chrono::steady_clock::now();

    while (cin >> line) {
        if (line.empty()) continue;
        Graph G;
        parse_graph6(line.c_str(), G);
        total_count++;

        if (check_leontovich(G, line)) {
            leontovich_count++;
        }

        if (total_count % 10000 == 0) {
            auto now = chrono::steady_clock::now();
            double secs = chrono::duration<double>(now - t0).count();
            int rate = secs > 0 ? (int)(total_count / secs) : 0;
            cerr << "  [filter] " << total_count << " graphs | "
                 << leontovich_count << " hits | " << rate << "/s | "
                 << (int)secs << "s elapsed\r" << flush;
        }
    }

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
