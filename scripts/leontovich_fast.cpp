// Fast asymptotic filter for Leontovich graphs using E_n^{(d)} trees.
// Compile: g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
// Usage:   geng -c 8 -q | ./leontovich_fast
//          genbg -c 7 8 -q | ./leontovich_fast   (bipartite m=15)
//
// v3: Precompute w_k = A^k * 1 once per graph, then compute all
//     hom(P_n) and hom(E_n^{(d)}) via dot products.  ~100-500x faster.

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static constexpr int MAX_V = 32;   // max vertices in target graph
static constexpr int MAX_K = 201;  // max walk length (n up to 200)

struct Graph {
    int m;                  // vertex count
    int deg[MAX_V];         // degree of each vertex
    int adj[MAX_V][MAX_V];  // adjacency list (neighbors)
};

static void parse_graph6(const char* g6, Graph& G) {
    if (strncmp(g6, ">>graph6<<", 10) == 0) g6 += 10;
    G.m = g6[0] - 63;
    memset(G.deg, 0, sizeof(G.deg));
    int k = 1, bit_pos = 5;
    for (int col = 1; col < G.m; col++) {
        for (int row = 0; row < col; row++) {
            int val = g6[k] - 63;
            if ((val >> bit_pos) & 1) {
                G.adj[row][G.deg[row]++] = col;
                G.adj[col][G.deg[col]++] = row;
            }
            if (--bit_pos < 0) {
                k++;
                bit_pos = 5;
            }
        }
    }
}

// Adjacency-list matvec: y = A * x  (skips zeros, ~2x faster than dense)
static inline void matvec_adj(const Graph& G, const double* __restrict x,
                              double* __restrict y) {
    for (int i = 0; i < G.m; i++) {
        double s = 0.0;
        for (int j = 0; j < G.deg[i]; j++) s += x[G.adj[i][j]];
        y[i] = s;
    }
}

// Returns true if graph is Leontovich (some E_n^d beats P_n).
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

    // hom(E_n^{(d)}, H) = sum_i  w[n-d-2][i] * w[1][i] * w[d][i]
    for (int d = 2; d <= 20; d++) {
        // branch factor b[i] = deg(i) * w[d][i]
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
