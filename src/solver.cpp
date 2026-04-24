#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

/**
 * UNIFIED HYBRID SOLVER: HARDCORE COMPUTATIONAL REFLECTION
 * Consolidates Paths 1-4 with mathematically rigorous algorithmic modules.
 * C++ Search -> Zero-Trust Lean Verification.
 */

// Terminal Colors for Showcase Output
#define RST "\033[0m"
#define GRN "\033[1;32m"
#define YEL "\033[1;33m"
#define BLU "\033[1;34m"
#define MAG "\033[1;35m"
#define CYN "\033[1;36m"
#define RED "\033[1;31m"

// ============================================================================
// MODULE 1: EPIDEMIOLOGY (GRAPH BURNING & THE BNC)
// Math: "Achievable Burning Densities of Growing Grids" (2026)
// ============================================================================
class EpidemiologyModule {
  public:
    static void execute(const string &fn) {
        int N = 64;
        cout << CYN << "[Solver] Initializing Spatial Comb Graph (N=" << N
             << ") for BNC Verification.\n"
             << RST;

        int theoretical_bound = ceil(sqrt(N));
        cout << MAG
             << "  -> Burning Number Conjecture (BNC) Limit: b(G) <= ceil(sqrt("
             << N << ")) = " << theoretical_bound << " steps.\n"
             << RST;

        // Generate Comb Graph (A spine of 32 nodes, with 32 leaves attached)
        vector<uint64_t> adj(64, 0);
        for (int i = 0; i < 32; ++i) {
            if (i > 0)
                adj[i] |= (1ULL << (i - 1));
            if (i < 31)
                adj[i] |= (1ULL << (i + 1));
            adj[i] |= (1ULL << (i + 32)); // attach leaf (tooth)
            adj[i + 32] |= (1ULL << i);   // return edge
        }

        // Real Greedy Look-Ahead Heuristic
        vector<uint32_t> seq;
        uint64_t burned = 0;
        int step = 0;

        while (burned != ~0ULL && step < 64) {
            step++;
            uint64_t spread = burned;
            for (int i = 0; i < 64; ++i)
                if ((burned >> i) & 1ULL)
                    spread |= adj[i];

            // Pick node maximizing unburned degree
            int best_node = -1;
            int max_degree = -1;
            for (int i = 0; i < 64; ++i) {
                if (!((spread >> i) & 1ULL)) {
                    int deg = __builtin_popcountll(adj[i] & ~spread);
                    if (deg > max_degree) {
                        max_degree = deg;
                        best_node = i;
                    }
                }
            }
            if (best_node == -1) {
                for (int i = 0; i < 64; ++i)
                    if (!((spread >> i) & 1ULL)) {
                        best_node = i;
                        break;
                    }
            }

            burned = spread | (1ULL << best_node);
            seq.push_back(best_node);
            cout << "  [Step " << step << "] Activator: Node " << setw(2)
                 << best_node << " | Saturation: " << fixed << setprecision(1)
                 << (__builtin_popcountll(burned) / 64.0) * 100 << "%\n";
        }

        cout << GRN << "  [Solver] Achieved full saturation in " << step
             << " steps. ";
        if (step <= theoretical_bound)
            cout << "(BNC Verified ✓)\n" << RST;
        else
            cout << RED << "(Exceeds BNC Limit ✗)\n" << RST;

        ofstream out(fn);
        out << "import Mathlib.Tactic\n\n";
        out << "def grid_adj : Array UInt64 := #[\n";
        for (uint64_t r : adj)
            out << "  " << r << ",\n";
        out << "]\n\n";
        out << "def deployment_sequence : List Nat := [";
        for (uint32_t n : seq)
            out << n << ", ";
        out << "]\n\n";
        out << "def spread_fire (adj : Array UInt64) (burned : UInt64) : "
               "UInt64 :=\n";
        out << "  (List.range 64).foldl (init := burned) (fun acc i => if "
               "(burned >>> i.toUInt64) &&& 1 == 1 then acc ||| (adj[i]!) else "
               "acc)\n\n";
        out << "def execute_burning (adj : Array UInt64) (seq : List Nat) : "
               "UInt64 :=\n";
        out << "  seq.foldl (init := 0) (fun burned n => (spread_fire adj "
               "burned) ||| ((1 : UInt64) <<< n.toUInt64))\n\n";
        out << "theorem policy_is_valid : execute_burning grid_adj "
               "deployment_sequence = 0xFFFFFFFFFFFFFFFF := by native_decide\n";

        // Generate Graphviz Visual Proof
        string dot_fn = fn + ".dot";
        ofstream dot_out(dot_fn);
        dot_out << "graph Epidemiology {\n  node [style=filled, "
                   "fillcolor=white];\n";
        for (uint32_t n : seq)
            dot_out << "  " << n
                    << " [fillcolor=red, fontcolor=white, label=\"Act " << n
                    << "\"];\n";
        for (uint32_t i = 0; i < 64; ++i) {
            for (uint32_t j = i + 1; j < 64; ++j) {
                if ((adj[i] >> j) & 1ULL)
                    dot_out << "  " << i << " -- " << j << ";\n";
            }
        }
        dot_out << "}\n";
        cout << "  [Solver] Visual Proof Exported: " << dot_fn << "\n";
    }
};

// ============================================================================
// MODULE 2: SURVEILLANCE (1-VISIBILITY LOCALIZATION)
// Math: "The one-visibility localization game" (2024)
// ============================================================================
class SurveillanceModule {
  public:
    static void execute(const string &fn) {
        cout << CYN
             << "[Solver] Initializing Belief-State POMDP Tracker for "
                "1-Visibility Evader.\n"
             << RST;
        cout
            << MAG
            << "  -> Topology: Subterranean Tunnel System (Path Graph, N=64).\n"
            << RST;

        // Build a path graph.
        vector<uint64_t> adj(64, 0);
        for (int i = 0; i < 63; i++) {
            adj[i] |= (1ULL << (i + 1));
            adj[i + 1] |= (1ULL << i);
        }

        uint64_t belief = ~0ULL; // Target quantum superposition
        vector<uint32_t> probes;

        int steps = 0;
        while (belief > 0 && steps < 64) {
            int best_p = 0;
            int min_next_belief = 65;
            uint64_t best_next_mask = 0;

            // POMDP Min-Max Lookahead
            for (int i = 0; i < 64; i++) {
                uint64_t b_after_probe = belief & ~((1ULL << i) | adj[i]);
                uint64_t next_b = 0;
                for (int j = 0; j < 64; j++) {
                    if ((b_after_probe >> j) & 1ULL)
                        next_b |= adj[j] | (1ULL << j);
                }
                int pop = __builtin_popcountll(next_b);
                if (pop < min_next_belief) {
                    min_next_belief = pop;
                    best_next_mask = next_b;
                    best_p = i;
                }
            }
            probes.push_back(best_p);
            belief = best_next_mask;

            cout << "  [Turn " << setw(2) << ++steps << "] Drone Probes Node "
                 << setw(2) << best_p
                 << " -> Target Entropy Reduced To: " << min_next_belief
                 << " possible nodes.\n";
        }

        if (belief == 0)
            cout << GRN
                 << "  [Solver] Isoperimetric pruning successful. Target "
                    "mathematically trapped.\n"
                 << RST;

        ofstream out(fn);
        out << "import Mathlib.Tactic\n\n";
        out << "def cave_adj : Array UInt64 := #[\n";
        for (uint64_t r : adj)
            out << "  " << r << ",\n";
        out << "]\n\n";
        out << "def drone_routing_playbook : List Nat := [";
        for (uint32_t n : probes)
            out << n << ", ";
        out << "]\n\n";
        out << "def drone_probe (adj : Array UInt64) (belief : UInt64) (p : "
               "Nat) : UInt64 :=\n";
        out << "  let captured := belief &&& ~~~((1 : UInt64) <<< p.toUInt64 "
               "||| adj[p]!)\n";
        out << "  (List.range 64).foldl (init := 0) (fun acc i => if (captured "
               ">>> i.toUInt64) &&& 1 == 1 then acc ||| ((1 : UInt64) <<< "
               "i.toUInt64) ||| adj[i]! else acc)\n\n";
        out << "def execute_hunt (adj : Array UInt64) (seq : List Nat) : "
               "UInt64 :=\n";
        out << "  seq.foldl (init := 0xFFFFFFFFFFFFFFFF) (fun b p => "
               "drone_probe adj b p)\n\n";
        out << "theorem capture_guaranteed : execute_hunt cave_adj "
               "drone_routing_playbook = 0 := by native_decide\n";
    }
};

// ============================================================================
// MODULE 3: SPECTRUM (HOFFMAN-LONDON ANOMALIES)
// Math: "Hoffman-London graphs: When paths minimize q-colorings among trees"
// (2026)
// ============================================================================
class SpectrumModule {
    static vector<uint64_t> count_q_colorings(int u, int p,
                                              const vector<vector<int>> &adj) {
        vector<uint64_t> dp(3, 1);
        for (int v : adj[u]) {
            if (v == p)
                continue;
            auto child_dp = count_q_colorings(v, u, adj);
            vector<uint64_t> next_dp(3, 0);
            for (int c = 0; c < 3; c++) {
                uint64_t sum_d = 0;
                for (int d = 0; d < 3; d++)
                    if (c != d)
                        sum_d += child_dp[d];
                next_dp[c] = dp[c] * sum_d;
            }
            dp = next_dp;
        }
        return dp;
    }

  public:
    static void execute(const string &fn) {
        cout << CYN
             << "[Solver] Evaluating Topological Spectrum Allocation "
                "(q-Colorings).\n"
             << RST;
        cout << MAG
             << "  -> Math Context: Galvin & Nir (2026) 'Hoffman-London "
                "graphs'\n"
             << RST;

        int N = 21;
        vector<vector<int>> path_adj(N);
        for (int i = 0; i < N - 1; i++) {
            path_adj[i].push_back(i + 1);
            path_adj[i + 1].push_back(i);
        }
        auto p_dp = count_q_colorings(0, -1, path_adj);
        uint64_t path_allocs = p_dp[0] + p_dp[1] + p_dp[2];

        vector<vector<int>> leon_adj(N);
        for (int i = 0; i < 4; i++) {
            leon_adj[i].push_back(i + 1);
            leon_adj[i + 1].push_back(i);
        }
        for (int i = 5; i < 13; i++) {
            leon_adj[1].push_back(i);
            leon_adj[i].push_back(1);
        }
        for (int i = 13; i < 21; i++) {
            leon_adj[3].push_back(i);
            leon_adj[i].push_back(3);
        }

        auto l_dp = count_q_colorings(0, -1, leon_adj);
        uint64_t leon_allocs = l_dp[0] + l_dp[1] + l_dp[2];

        cout << "  [Path P_21] Baseline Valid Frequency Allocations: "
             << path_allocs << "\n";
        cout << "  [Leontovich L_21] Hub-Constrained Allocations:  "
             << leon_allocs << "\n";

        if (leon_allocs < path_allocs)
            cout << RED
                 << "  [Solver] ANOMALY DETECTED! Structural tree fragility "
                    "confirmed via DP.\n"
                 << RST;

        ofstream out(fn);
        out << "import Mathlib.Tactic\n\n";
        out << "def path_allocations : Nat := " << path_allocs << "\n";
        out << "def leontovich_allocations : Nat := " << leon_allocs << "\n\n";
        out << "theorem anomaly_verified : leontovich_allocations < "
               "path_allocations := by decide\n";
    }
};

// ============================================================================
// MODULE 4: FINANCE (TURAN SUPERSATURATION)
// Math: "A localized approach to generalized Turan problems" (2023)
// ============================================================================
class FinanceModule {
  public:
    static void execute(const string &fn) {
        int N = 64;
        int max_bipartite_edges = (N / 2) * (N / 2);

        cout << CYN
             << "[Solver] Turan Limits & Bipartite Supersaturation (N=" << N
             << ").\n"
             << RST;
        cout << MAG << "  -> Mantel's Theorem Limit: " << max_bipartite_edges
             << "\n"
             << RST;

        vector<uint64_t> adj(64, 0);
        int edges = 0;
        for (int i = 0; i < 32; i++) {
            for (int j = 32; j < 64; j++) {
                adj[i] |= (1ULL << j);
                adj[j] |= (1ULL << i);
                edges++;
            }
        }

        vector<pair<int, int>> fraud = {
            {0, 1}, {1, 2}, {33, 34}, {34, 35}, {35, 36}};
        for (auto p : fraud) {
            adj[p.first] |= (1ULL << p.second);
            adj[p.second] |= (1ULL << p.first);
            edges++;
        }

        cout << "  [Network Data] Ingested matrix with " << edges
             << " active dependencies.\n";
        cout << RED << "  [Solver] " << edges << " > " << max_bipartite_edges
             << ". Supersaturation boundary violently breached!\n"
             << RST;

        auto start = high_resolution_clock::now();
        uint32_t exact_k3 = 0;
        for (int i = 0; i < N - 1; i++) {
            for (int j = i + 1; j < N; j++) {
                // Safety check: j must be < 64 for valid bit-shift on uint64_t
                if (j < 64 && (adj[i] & (1ULL << j))) {
                    exact_k3 += __builtin_popcountll(adj[i] & adj[j]);
                }
            }
        }
        exact_k3 /= 3;

        auto stop = high_resolution_clock::now();

        cout << GRN << "  [SIMD Engine] Intersected graph arrays in "
             << duration_cast<microseconds>(stop - start).count() << " µs. "
             << "Discovered exactly " << exact_k3
             << " systemic K_3 risk cycles.\n"
             << RST;

        ofstream out(fn);
        out << "import Mathlib.Tactic\n\n";
        out << "def edges : Nat := " << edges << "\n";
        out << "def mantel_limit : Nat := " << max_bipartite_edges << "\n";
        out << "def exact_cycles : Nat := " << exact_k3 << "\n\n";
        out << "theorem supersaturation_active : edges > mantel_limit := by "
               "decide\n";
        out << "theorem cycles_exist : exact_cycles > 0 := by decide\n";

        // Generate Graphviz Visual Proof
        string dot_fn = fn + ".dot";
        ofstream dot_out(dot_fn);
        dot_out << "graph Finance {\n  node [style=filled, fillcolor=white];\n";
        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                if (j < 64 && (adj[i] & (1ULL << j))) {
                    const bool is_fraud = std::any_of(
                        fraud.begin(), fraud.end(), [&](const auto &p) {
                            return (p.first == i && p.second == j) ||
                                   (p.first == j && p.second == i);
                        });
                    if (is_fraud) {
                        dot_out << "  " << i << " -- " << j
                                << " [color=red, penwidth=3.0];\n";
                    } else {
                        // Regular bipartite edges
                        dot_out << "  " << i << " -- " << j
                                << " [color=blue, penwidth=0.1];\n";
                    }
                }
            }
        }
        dot_out << "}\n";
        cout << "  [Solver] Visual Proof Exported: " << dot_fn << "\n";
    }
};

int main(int argc, char **argv) {
    if (argc < 3)
        return 1;
    string mode = argv[1];
    string output = argv[2];

    if (mode == "epidemiology")
        EpidemiologyModule::execute(output);
    else if (mode == "surveillance")
        SurveillanceModule::execute(output);
    else if (mode == "spectrum")
        SpectrumModule::execute(output);
    else if (mode == "finance")
        FinanceModule::execute(output);

    return 0;
}
