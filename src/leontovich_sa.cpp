#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,fma")
// Simulated annealing search for small Leontovich graphs.
// Compile: g++ -O3 -march=native -std=c++17 -o leontovich_sa
// src/leontovich_sa.cpp Usage:   ./leontovich_sa --steps 1000000 --seed 42
// --start 7,1,9
//          ./leontovich_sa --steps 1000000 --seed 42 --start L76

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

static constexpr int MAXV = 128;
static constexpr int MAX_K = 301;

namespace sa {

struct Graph {
    int m;
    double A[MAXV][MAXV];

    void clear() {
        m = 0;
        memset(A, 0, sizeof(A));
    }

    void add_edge(int u, int v) { A[u][v] = A[v][u] = 1.0; }
};

// ── Build T(x,y,z): root → x children → x bridges → x*z leaves ────────
static Graph make_T(int x, int y, int z) {
    Graph G;
    G.clear();
    int id = 0;
    int root = id++;
    std::vector<int> children;
    for (int i = 0; i < x; i++) {
        int c = id++;
        G.add_edge(root, c);
        children.push_back(c);
    }
    for (int i = 0; i < x; i++) {
        int hub = id++;
        G.add_edge(children[i], hub);
        for (int j = 0; j < z; j++) {
            int leaf = id++;
            G.add_edge(hub, leaf);
        }
    }
    G.m = id;
    return G;
}

// ── Load graph from JSON edge list ─────────────────────────────────────
static Graph load_json(const std::string& path) {
    Graph G;
    G.clear();
    std::ifstream f(path);
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());

    // Parse "vertices": N
    auto vp = content.find("\"vertices\"");
    if (vp != std::string::npos) {
        auto colon = content.find(':', vp);
        G.m = std::stoi(content.substr(colon + 1));
    }

    // Parse "edges": [[a,b],...]
    auto ep = content.find("\"edges\"");
    if (ep != std::string::npos) {
        auto bracket = content.find('[', ep + 7);
        size_t pos = bracket;
        while (pos < content.size()) {
            auto ob = content.find('[', pos + 1);
            if (ob == std::string::npos) break;
            auto comma = content.find(',', ob);
            auto cb = content.find(']', comma);
            if (cb == std::string::npos) break;
            int u = std::stoi(content.substr(ob + 1, comma - ob - 1));
            int v = std::stoi(content.substr(comma + 1, cb - comma - 1));
            G.add_edge(u, v);
            pos = cb;
            // Check if next non-space is ] (end of outer array)
            size_t np = content.find_first_not_of(" \n\r\t", pos + 1);
            if (np != std::string::npos && content[np] == ']') break;
        }
    }
    return G;
}

// ── Check Leontovich crossover ─────────────────────────────────────────
struct LeoResult {
    bool is_leo;
    int n;
    int d;
    double ratio;
};

static LeoResult check_leontovich(const Graph& G) {
    int m = G.m;
    // Compute w[k] = A^k * 1
    static double w[MAX_K][MAXV];
    static double homP[MAX_K + 1];

    memset(w, 0, sizeof(w));
    memset(homP, 0, sizeof(homP));
    for (int i = 0; i < m; i++) w[0][i] = 1.0;
    homP[1] = m;

    for (int step = 1; step < MAX_K; step++) {
        double tmp[MAXV] = {};
        for (int j = 0; j < m; j++) {
            double wj = w[step - 1][j];
            for (int i = 0; i < m; i++) {
                tmp[i] += G.A[j][i] * wj;
            }
        }
        double s = 0.0;
        for (int i = 0; i < m; i++) {
            w[step][i] = tmp[i];
            s += tmp[i];
        }
        // Normalize to prevent overflow
        if (s > 1e100) {
            double inv = 1.0 / s;
            for (int i = 0; i < m; i++) w[step][i] *= inv;
            s = 1.0;
        }
        homP[step + 1] = s;
    }

    LeoResult best = {false, 0, 0, 1.0};

    for (int d = 2; d <= 20; d++) {
        double b[MAXV] = {};
        for (int i = 0; i < m; i++) {
            b[i] = w[1][i] * w[d][i];
        }

        int limit = std::min(MAX_K - d - 2, 298);
        for (int stem = 0; stem <= limit; stem++) {
            double homE = 0.0;
            for (int i = 0; i < m; i++) {
                homE += w[stem][i] * b[i];
            }
            int n = stem + d + 2;
            double ratio = homP[n] > 0 ? homE / homP[n] : 1.0;
            if (ratio < 1.0 - 1e-11) {
                if (!best.is_leo || ratio < best.ratio) {
                    best = {true, n, d, ratio};
                }
            }
        }
    }
    return best;
}

// ── Connectivity check (BFS) ───────────────────────────────────────────
static bool is_connected(const Graph& G) {
    bool visited[MAXV] = {};
    int queue[MAXV];
    int head = 0, tail = 0;
    visited[0] = true;
    queue[tail++] = 0;
    while (head < tail) {
        int u = queue[head++];
        for (int v = 0; v < G.m; v++) {
            if (G.A[u][v] > 0 && !visited[v]) {
                visited[v] = true;
                queue[tail++] = v;
            }
        }
    }
    return tail == G.m;
}

// ── Mutate: flip a random edge ─────────────────────────────────────────
static Graph mutate(const Graph& G, std::mt19937& rng) {
    Graph H = G;
    std::uniform_int_distribution<int> dist(0, G.m - 1);

    for (int tries = 0; tries < 50; tries++) {
        int u = dist(rng), v = dist(rng);
        if (u == v) continue;

        if (H.A[u][v] > 0) {
            // Try removing edge
            H.A[u][v] = H.A[v][u] = 0.0;
            if (is_connected(H)) return H;
            H.A[u][v] = H.A[v][u] = 1.0;  // revert
        } else {
            // Add edge
            H.A[u][v] = H.A[v][u] = 1.0;
            return H;
        }
    }
    return G;  // no valid mutation found
}

// ── Shrink: remove a random degree-1 vertex ────────────────────────────
static Graph shrink(const Graph& G, uint32_t rand_val) {
    // Find degree-1 vertices
    std::vector<int> leaves;
    for (int i = 0; i < G.m; i++) {
        int deg = 0;
        for (int j = 0; j < G.m; j++) deg += (int)G.A[i][j];
        if (deg == 1) leaves.push_back(i);
    }
    if (leaves.empty()) return G;

    // Pick leaf using provided random value
    int leaf = leaves[rand_val % leaves.size()];

    // Build new graph without this vertex
    Graph H;
    H.clear();
    H.m = G.m - 1;
    int remap[MAXV];
    int id = 0;
    for (int i = 0; i < G.m; i++) {
        remap[i] = (i == leaf) ? -1 : id++;
    }
    for (int i = 0; i < G.m; i++) {
        if (remap[i] < 0) continue;
        for (int j = i + 1; j < G.m; j++) {
            if (remap[j] < 0) continue;
            if (G.A[i][j] > 0) H.add_edge(remap[i], remap[j]);
        }
    }
    return H;
}

// ── Print edges as JSON array ──────────────────────────────────────────
static std::string edges_json(const Graph& G) {
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (int i = 0; i < G.m; i++) {
        for (int j = i + 1; j < G.m; j++) {
            if (G.A[i][j] > 0) {
                if (!first) ss << ",";
                ss << "[" << i << "," << j << "]";
                first = false;
            }
        }
    }
    ss << "]";
    return ss.str();
}

// ── Spectral ratio via power iteration on A^2 ─────────────────────────
static void spectral(const Graph& G, double& lam1, double& lam2) {
    int m = G.m;
    // Use walk-count growth rates: w[k] = A^k * 1
    // lambda_1 = lim_{k->inf} (sum w[k])^{1/k}
    // For lambda_2, use the second eigenvector via deflation on A directly.

    // Compute w[k] and extract lambda_1 from growth rate
    double w[60][MAXV];
    memset(w, 0, sizeof(w));
    for (int i = 0; i < m; i++) w[0][i] = 1.0;
    for (int step = 1; step < 60; step++) {
        for (int j = 0; j < m; j++) {
            double wj = w[step - 1][j];
            for (int i = 0; i < m; i++) {
                w[step][i] += G.A[j][i] * wj;
            }
        }
    }

    // lambda_1 from ratio of consecutive sums (even steps for bipartite)
    double s58 = 0, s56 = 0;
    for (int i = 0; i < m; i++) {
        s58 += w[58][i];
        s56 += w[56][i];
    }
    lam1 = (s56 > 0) ? std::sqrt(s58 / s56) : 0;

    // lambda_2 via power iteration on A with deflation
    double v1[MAXV];
    // First eigenvector via power iteration on A (even steps)
    double u[MAXV];
    for (int i = 0; i < m; i++) u[i] = w[58][i];
    double norm = 0;
    for (int i = 0; i < m; i++) norm += u[i] * u[i];
    norm = std::sqrt(norm);
    for (int i = 0; i < m; i++) v1[i] = u[i] / norm;

    // Seed v2 orthogonal to v1
    double v[MAXV];
    for (int i = 0; i < m; i++) v[i] = (double)(i * 7 % 13) - 6.0;
    double dot = 0;
    for (int i = 0; i < m; i++) dot += v[i] * v1[i];
    for (int i = 0; i < m; i++) v[i] -= dot * v1[i];

    // Power iterate A^2 with deflation for second eigenvalue
    double A2[MAXV][MAXV] = {};
    for (int i = 0; i < m; i++)
        for (int k = 0; k < m; k++)
            if (G.A[i][k] > 0)
                for (int j = 0; j < m; j++) A2[i][j] += G.A[k][j];

    // Also deflate the -lambda_1 eigenvector (bipartite partner)
    // For bipartite graphs, the -lambda_1 eigenvector of A becomes
    // a lambda_1^2 eigenvector of A^2. We need to deflate both.
    double v1neg[MAXV];
    for (int i = 0; i < m; i++) v1neg[i] = w[59][i];  // odd step
    norm = 0;
    for (int i = 0; i < m; i++) norm += v1neg[i] * v1neg[i];
    norm = std::sqrt(norm);
    if (norm > 1e-15)
        for (int i = 0; i < m; i++) v1neg[i] /= norm;

    double eigenval = 0;
    for (int iter = 0; iter < 200; iter++) {
        // Orthogonalize against v1 and v1neg
        dot = 0;
        for (int i = 0; i < m; i++) dot += v[i] * v1[i];
        for (int i = 0; i < m; i++) v[i] -= dot * v1[i];
        dot = 0;
        for (int i = 0; i < m; i++) dot += v[i] * v1neg[i];
        for (int i = 0; i < m; i++) v[i] -= dot * v1neg[i];

        double nv[MAXV] = {};
        for (int i = 0; i < m; i++)
            for (int j = 0; j < m; j++) nv[i] += A2[i][j] * v[j];

        // Orthogonalize result
        dot = 0;
        for (int i = 0; i < m; i++) dot += nv[i] * v1[i];
        for (int i = 0; i < m; i++) nv[i] -= dot * v1[i];
        dot = 0;
        for (int i = 0; i < m; i++) dot += nv[i] * v1neg[i];
        for (int i = 0; i < m; i++) nv[i] -= dot * v1neg[i];

        norm = 0;
        for (int i = 0; i < m; i++) norm += nv[i] * nv[i];
        norm = std::sqrt(norm);
        if (norm < 1e-15) break;
        eigenval = 0;
        for (int i = 0; i < m; i++) {
            v[i] = nv[i] / norm;
            eigenval += nv[i] * v[i];
        }
    }
    lam2 = std::sqrt(std::abs(eigenval));
}

}  // namespace sa

int main(int argc, char** argv) {
    using namespace sa;
    // Parse args
    int steps = 1000000;
    int seed = 42;
    double temp_init = 2.0;
    std::string start = "7,1,9";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cerr
                << "Usage: leontovich_sa [OPTIONS]\n"
                << "  --steps N     SA steps (default: 1000000)\n"
                << "  --seed N      RNG seed (default: 42)\n"
                << "  --temp F      Initial temperature (default: 2.0)\n"
                << "  --start STR   Seed graph: x,y,z for T(x,y,z) or L76\n";
            return 0;
        } else if (arg == "--steps" && i + 1 < argc)
            steps = std::stoi(argv[++i]);
        else if (arg == "--seed" && i + 1 < argc)
            seed = std::stoi(argv[++i]);
        else if (arg == "--temp" && i + 1 < argc)
            temp_init = std::stod(argv[++i]);
        else if (arg == "--start" && i + 1 < argc)
            start = argv[++i];
    }

    std::mt19937 rng(seed);

    // Build seed graph
    Graph G;
    std::string seed_name;
    if (start == "L76") {
        // Find JSON relative to binary
        G = load_json("data/leontovich_76.json");
        seed_name = "L76";
    } else {
        // Parse x,y,z
        int x, y, z;
        sscanf(start.c_str(), "%d,%d,%d", &x, &y, &z);
        G = make_T(x, y, z);
        seed_name = "T(" + start + ")";
    }

    auto leo = check_leontovich(G);
    double lam1, lam2;
    spectral(G, lam1, lam2);
    double r = (lam1 > 0) ? lam2 / lam1 : 0;

    std::cerr << "=== Leontovich SA [C++] ===" << std::endl;
    std::cerr << "Steps: " << steps << ", Temp: " << temp_init
              << ", Seed: " << seed_name << ", |V|=" << G.m
              << ", leo=" << leo.is_leo << ", r=" << std::fixed
              << std::setprecision(4) << r << std::endl;

    // Score function
    auto score = [](const Graph& g, const LeoResult& lr) -> double {
        if (lr.is_leo) return (double)g.m;
        return 500.0 + g.m + 100.0 * std::max(0.0, lr.ratio - 0.999);
    };

    double current_score = score(G, leo);
    int best_m = G.m;
    Graph best_G = G;
    auto t0 = std::chrono::steady_clock::now();

    for (int step = 0; step < steps; step++) {
        double temp = temp_init * (1.0 - (double)step / steps);

        // Mutate or shrink
        Graph H;
        if (rng() % 10 == 0) {
            H = shrink(G, rng());
        } else {
            H = mutate(G, rng);
        }

        auto leo_h = check_leontovich(H);
        double h_score = score(H, leo_h);

        double delta = h_score - current_score;
        std::uniform_real_distribution<double> u01(0.0, 1.0);
        if (delta < 0 ||
            (temp > 0 && u01(rng) < std::exp(-delta / std::max(temp, 0.01)))) {
            G = H;
            current_score = h_score;
            leo = leo_h;

            if (leo_h.is_leo && H.m < best_m) {
                best_m = H.m;
                best_G = H;
                spectral(H, lam1, lam2);
                r = lam2 / lam1;

                std::cout << "{\"step\":" << step << ",\"m\":" << H.m
                          << ",\"n\":" << leo_h.n << ",\"d\":" << leo_h.d
                          << ",\"ratio\":" << std::fixed << std::setprecision(8)
                          << leo_h.ratio << ",\"r\":" << std::setprecision(6)
                          << r << ",\"lam1\":" << lam1 << ",\"lam2\":" << lam2
                          << ",\"edges\":" << edges_json(H) << "}" << std::endl;

                std::cerr << "  [SA] step=" << step << " NEW BEST |V|=" << H.m
                          << " n=" << leo_h.n << " d=" << leo_h.d
                          << " r=" << std::setprecision(4) << r << std::endl;
            }
        }

        if (step % 10000 == 0) {
            auto now = std::chrono::steady_clock::now();
            double secs = std::chrono::duration<double>(now - t0).count();
            int rate = secs > 0 ? (int)(step / secs) : 0;
            std::cerr << "  [SA] step=" << step << "/" << steps
                      << " |V|=" << G.m << " leo=" << leo.is_leo
                      << " best=" << best_m << " T=" << std::setprecision(4)
                      << temp << " " << rate << " steps/s\r" << std::flush;
        }
    }

    std::cerr << "\n=== Final: best |V|=" << best_m << " ===" << std::endl;
    return 0;
}
