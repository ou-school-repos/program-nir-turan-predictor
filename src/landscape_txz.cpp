// Exhaustive threshold landscape for T(x,1,z).
// For each odd threshold n, find the smallest T(x,1,z) that first crosses over.
//
// Usage: ./landscape_txz [max_V]    (default: 50000)
//
// Build: g++ -O2 -o landscape_txz src/landscape_txz.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

static const int K = 4;      // quotient matrix dimension
static const int NMAX = 51;  // max source tree size to check

// hom(P_n, H) via M^{n-1} . 1, weighted by orbit sizes a
// hom(E_n^{(2)}, H): path with pendant at depth 2
// Uses the eval_tree_hom approach from leontovich.py but specialized for 4x4.

struct Vec {
    double v[K];
};
struct Mat {
    double m[K][K];
};

static Vec matvec(const Mat& M, const Vec& x) {
    Vec r = {};
    for (int i = 0; i < K; i++)
        for (int j = 0; j < K; j++) r.v[i] += M.m[i][j] * x.v[j];
    return r;
}

// Compute hom(P_n, H) = a . M^{n-1} . 1
static double hom_path(const Mat& M, const double a[K], int n) {
    Vec w = {{1, 1, 1, 1}};
    for (int step = 0; step < n - 1; step++) w = matvec(M, w);
    double h = 0;
    for (int i = 0; i < K; i++) h += a[i] * w.v[i];
    return h;
}

// Compute hom(E_n^{(2)}, H) using bottom-up DP on the tree structure:
// E_n = P_{n-1} with pendant at vertex (n-4) from one end.
// Uses the standard eval_tree_hom approach with the quotient matrix.
static double hom_en(const Mat& M, const double a[K], int n) {
    if (n < 5) return hom_path(M, a, n);

    // E_n has n vertices: path 0-1-2-...(n-2), pendant (n-1) attached to vertex
    // (n-4). Bottom-up DP: dp[v][c] = product over children of sum_d
    // M[c][d]*dp[child][d] Process leaves first, then work up.

    // For the quotient matrix, the tree is small (n vertices), and DP is O(n *
    // K^2).
    int parent[NMAX + 1];
    int adj[NMAX + 1][3];  // max degree 3 (branch vertex)
    int deg[NMAX + 1];
    memset(deg, 0, sizeof(deg));
    memset(parent, -1, sizeof(parent));

    // Build adjacency
    for (int i = 0; i < n - 2; i++) {
        int u = i, v = i + 1;
        adj[u][deg[u]++] = v;
        adj[v][deg[v]++] = u;
    }
    // pendant edge: (n-4) -- (n-1)
    adj[n - 4][deg[n - 4]++] = n - 1;
    adj[n - 1][deg[n - 1]++] = n - 4;

    // BFS from vertex 0
    int order[NMAX + 1], head = 0, tail = 0;
    bool visited[NMAX + 1] = {};
    order[tail++] = 0;
    visited[0] = true;
    while (head < tail) {
        int u = order[head++];
        for (int i = 0; i < deg[u]; i++) {
            int v = adj[u][i];
            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                order[tail++] = v;
            }
        }
    }

    // Bottom-up DP
    double dp[NMAX + 1][K];
    for (int i = 0; i < n; i++)
        for (int c = 0; c < K; c++) dp[i][c] = 1.0;

    for (int idx = n - 1; idx >= 1; idx--) {
        int u = order[idx];
        int p = parent[u];
        for (int cp = 0; cp < K; cp++) {
            double s = 0;
            for (int d = 0; d < K; d++) s += M.m[cp][d] * dp[u][d];
            dp[p][cp] *= s;
        }
    }

    double h = 0;
    for (int i = 0; i < K; i++) h += a[i] * dp[0][i];
    return h;
}

int main(int argc, char** argv) {
    int max_v = 50000;
    if (argc > 1) max_v = atoi(argv[1]);

    printf("Task F: Threshold landscape for T(x,1,z), |V| <= %d\n", max_v);

    std::map<int, std::tuple<int, int, int>> results;  // threshold -> (x, z, V)
    long long count = 0;

    for (int x = 2;; x++) {
        if (1 + 2 * x + x * 2 > max_v) break;
        for (int z = 2;; z++) {
            int V = 1 + 2 * x + x * z;
            if (V > max_v) break;
            count++;

            Mat M = {};
            M.m[0][1] = x;
            M.m[1][0] = 1;
            M.m[1][2] = 1;
            M.m[2][1] = 1;
            M.m[2][3] = z;
            M.m[3][2] = 1;
            double a[K] = {1.0, (double)x, (double)x, (double)(x * z)};

            for (int n = 5; n <= NMAX; n += 2) {
                double he = hom_en(M, a, n);
                double hp = hom_path(M, a, n);
                if (he < hp) {
                    auto it = results.find(n);
                    if (it == results.end() || V < std::get<2>(it->second))
                        results[n] = {x, z, V};
                    break;
                }
            }
        }
    }

    printf("\nSearched %lld T(x,1,z) graphs\n\n", count);
    printf("%-14s %-22s %10s\n", "Threshold n", "Smallest T(x,1,z)", "|V|");
    printf("------------------------------------------------\n");
    int max_found = 0;
    for (auto& [t, v] : results) {
        auto [x, z, V] = v;
        char label[64];
        snprintf(label, sizeof(label), "T(%d,1,%d)", x, z);
        printf("  n=%-10d %-22s %10d\n", t, label, V);
        if (t > max_found) max_found = t;
    }
    if (max_found < NMAX)
        printf("\nNo threshold >= %d found with |V| <= %d\n", max_found + 2,
               max_v);

    return 0;
}
