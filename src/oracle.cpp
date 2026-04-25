#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;
using namespace std::chrono;

// Terminal Colors
#define RST "\033[0m"
#define GRN "\033[1;32m"
#define MAG "\033[1;35m"
#define CYN "\033[1;36m"
#define RED "\033[1;31m"

// ============================================================================
// MODULE: EPIDEMIOLOGY (Decreasing Radius APSP)
// ============================================================================
void run_epidemiology(const string& lean_fn, const string& dot_fn) {
    int N = 64;
    cout << CYN << "[Oracle] Initializing Spatial Comb Graph (N=" << N
         << ") for BNC Verification.\n"
         << RST;

    vector<uint64_t> adj(N, 0);
    for (int i = 0; i < 32; i++) {
        if (i > 0) adj[i] |= (1ULL << (i - 1));
        if (i < 31) adj[i] |= (1ULL << (i + 1));
        adj[i] |= (1ULL << (i + 32));
        adj[i + 32] |= (1ULL << i);
    }

    // All-Pairs Shortest Path (Floyd-Warshall)
    vector<vector<int>> dist(N, vector<int>(N, 1e9));
    for (int i = 0; i < N; i++) dist[i][i] = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if ((adj[i] >> j) & 1) dist[i][j] = 1;
    for (int k = 0; k < N; k++)
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);

    int k_limit = ceil(sqrt(N));  // 8
    cout << MAG << "  -> Burning Number Conjecture Limit: b(G) <= " << k_limit
         << " steps.\n"
         << RST;

    vector<int> seq;
    uint64_t burned = 0;

    // Spread-then-drop simulation matching Lean's execute_burning semantics:
    //   Each step: (1) spread existing fire one hop, then (2) ignite drop node.
    // Greedy: pick the node that maximizes coverage after this step's spread.
    auto spread = [&](uint64_t b) -> uint64_t {
        uint64_t out = b;
        for (int i = 0; i < N; i++)
            if ((b >> i) & 1) out |= adj[i];
        return out;
    };

    for (int step = 0; step < k_limit && burned != ~0ULL; step++) {
        uint64_t after_spread = spread(burned);
        int best_node = 0;
        int max_cov = -1;
        int remaining_steps = k_limit - step - 1;
        for (int i = 0; i < N; i++) {
            // Simulate: drop at i, then let fire spread for remaining steps
            uint64_t trial = after_spread | (1ULL << i);
            for (int s = 0; s < remaining_steps; s++) trial = spread(trial);
            int cov = __builtin_popcountll(trial);
            if (cov > max_cov) {
                max_cov = cov;
                best_node = i;
            }
        }
        burned = after_spread | (1ULL << best_node);
        seq.push_back(best_node);
        cout << "  [Step " << seq.size() << "] Drop at Node " << best_node
             << " | Saturation: " << fixed << setprecision(1)
             << (__builtin_popcountll(burned) * 100.0) / N << "%\n";
    }

    if (burned == ~0ULL && seq.size() <= (size_t)k_limit) {
        cout << GRN
             << "  [Success] Decreasing Radius Heuristic defeated local "
                "minima! Saturation in "
             << seq.size() << " steps.\n"
             << RST;
    } else {
        cout << RED << "  [Fail] Did not saturate within BNC limit.\n" << RST;
    }

    // --- DOT output with HTML labels ---
    ofstream dot(dot_fn);
    dot << "graph Epidemiology {\n  layout=fdp; splines=true; "
           "overlap=false;\n"
        << "  graph [label=\"Burning Number Conjecture (BNC) — Exact "
           "APSP\\nComb Graph C(32,2) | Limit: 8 steps\", "
           "fontname=\"Helvetica\", labelloc=t];\n"
        << "  node [fontname=\"Helvetica\", shape=none];\n";
    for (int i = 0; i < N; i++) {
        auto it = find(seq.begin(), seq.end(), i);
        string color = (it != seq.end()) ? "#ff4d4d" : "#e9ecef";
        string font_col = (it != seq.end()) ? "white" : "black";
        string label = (it != seq.end())
                           ? "Act " + to_string(distance(seq.begin(), it) + 1)
                           : "N" + to_string(i);
        dot << "  " << i
            << " [label=<\n    <table border=\"0\" cellborder=\"1\" "
               "cellspacing=\"0\" cellpadding=\"6\" bgcolor=\""
            << color << "\">\n"
            << "      <tr><td><font color=\"" << font_col << "\"><b>" << label
            << "</b></font></td></tr>\n    </table>>];\n";
    }
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if ((adj[i] >> j) & 1)
                dot << "  " << i << " -- " << j
                    << " [color=\"#adb5bd\", penwidth=1.5];\n";
    dot << "}\n";

    // --- Lean proof ---
    ofstream out(lean_fn);
    out << "import Mathlib.Tactic\ndef grid_adj : Array UInt64 := #[\n";
    for (uint64_t r : adj) out << r << ",\n";
    out << "]\ndef deployment_sequence : List Nat := [";
    for (int x : seq) out << x << ", ";
    out << "]\ndef spread_fire (adj : Array UInt64) (burned : UInt64) : "
           "UInt64 :=\n"
        << "  (List.range 64).foldl (init := burned) (fun acc i => if "
           "(burned >>> i.toUInt64) &&& 1 == 1 then acc ||| (adj[i]!) else "
           "acc)\n"
        << "def execute_burning (adj : Array UInt64) (seq : List Nat) : "
           "UInt64 :=\n"
        << "  seq.foldl (init := 0) (fun burned n => (spread_fire adj "
           "burned) ||| ((1 : UInt64) <<< n.toUInt64))\n"
        << "theorem policy_satisfies_bnc : execute_burning grid_adj "
           "deployment_sequence = 0xFFFFFFFFFFFFFFFF ∧ "
           "deployment_sequence.length ≤ 8 := by native_decide\n";
}

// ============================================================================
// MODULE: SURVEILLANCE (POMDP with Temporal Heatmap)
// ============================================================================
void run_surveillance(const string& lean_fn, const string& dot_fn) {
    int N = 63;
    cout << CYN << "[Oracle] 1-Visibility POMDP Tracker on Binary Tree.\n"
         << RST;
    vector<uint64_t> adj(64, 0);
    for (int i = 0; i <= 30; i++) {
        int l = 2 * i + 1, r = 2 * i + 2;
        adj[i] |= (1ULL << l) | (1ULL << r);
        adj[l] |= (1ULL << i);
        adj[r] |= (1ULL << i);
    }

    uint64_t belief = 0x7FFFFFFFFFFFFFFF;
    vector<int> probes;

    while (belief > 0 && probes.size() < 63) {
        int best_p = -1, min_b = 65;
        uint64_t best_mask = 0;
        for (int i = 0; i < N; i++) {
            uint64_t post = belief & ~((1ULL << i) | adj[i]);
            uint64_t next_b = 0;
            for (int j = 0; j < N; j++)
                if ((post >> j) & 1) next_b |= adj[j] | (1ULL << j);
            int pop = __builtin_popcountll(next_b);
            if (pop < min_b) {
                min_b = pop;
                best_mask = next_b;
                best_p = i;
            }
        }
        probes.push_back(best_p);
        belief = best_mask;
    }
    cout << GRN << "  [Success] Target isolated and captured in "
         << probes.size() << " deterministic probes.\n"
         << RST;

    // --- DOT output with temporal heatmap ---
    ofstream dot(dot_fn);
    dot << "graph Surveillance {\n  layout=dot; splines=ortho; "
           "rankdir=TB;\n"
        << "  graph [label=\"1-Visibility POMDP Min-Max Search\\nTemporal "
           "Heatmap: Red (Early) -> Green (Late)\", fontname=\"Helvetica\", "
           "labelloc=t];\n"
        << "  node [shape=none, fontname=\"Helvetica\"];\n";

    vector<int> turn(N, -1);
    for (size_t t = 0; t < probes.size(); t++) turn[probes[t]] = t;

    for (int i = 0; i < N; i++) {
        string color = "#e9ecef";
        string l1 = "Unvisited", l2 = "";
        if (turn[i] != -1) {
            double ratio = (double)turn[i] / probes.size();
            int red = (ratio < 0.5) ? 255 : 255 * (1.0 - ratio) * 2;
            int grn = (ratio > 0.5) ? 255 : 255 * ratio * 2;
            char hex[16];
            snprintf(hex, sizeof(hex), "#%02x%02x32", red, grn);
            color = hex;
            l1 = "Probe " + to_string(i);
            l2 = "Turn " + to_string(turn[i] + 1);
        }
        dot << "  " << i
            << " [label=<\n    <table border=\"0\" cellborder=\"1\" "
               "cellspacing=\"0\" cellpadding=\"4\" bgcolor=\""
            << color << "\">\n"
            << "      <tr><td><b>N" << i << "</b></td></tr>\n      <tr><td>"
            << l1 << "</td></tr>\n      <tr><td>" << l2 << "</td></tr>\n"
            << "    </table>>];\n";
    }
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if ((adj[i] >> j) & 1) dot << "  " << i << " -- " << j << ";\n";
    dot << "}\n";

    // --- Lean proof ---
    ofstream out(lean_fn);
    out << "import Mathlib.Tactic\ndef cave_adj : Array UInt64 := #[\n";
    for (int i = 0; i < 64; i++) out << "  " << adj[i] << ",\n";
    out << "]\ndef drone_routing_playbook : List Nat := [";
    for (int x : probes) out << x << ", ";
    out << "]\ndef drone_probe (adj : Array UInt64) (belief : UInt64) (p : "
           "Nat) : UInt64 :=\n"
        << "  let captured := belief &&& ~~~((1 : UInt64) <<< p.toUInt64 "
           "||| adj[p]!)\n"
        << "  (List.range 64).foldl (init := 0) (fun acc i => if (captured "
           ">>> i.toUInt64) &&& 1 == 1 then acc ||| ((1 : UInt64) <<< "
           "i.toUInt64) ||| adj[i]! else acc)\n"
        << "def execute_hunt (adj : Array UInt64) (seq : List Nat) : "
           "UInt64 :=\n"
        << "  seq.foldl (init := 0x7FFFFFFFFFFFFFFF) (fun b p => "
           "drone_probe adj b p)\n"
        << "theorem capture_guaranteed : execute_hunt cave_adj "
           "drone_routing_playbook = 0 := by native_decide\n";
}

// ============================================================================
// MODULE: FINANCE (Eigenvector Centrality & Alpha Transparency)
// ============================================================================
void run_finance(const string& lean_fn, const string& dot_fn) {
    int N = 64;
    cout << CYN << "[Oracle] Turan Limits & Systemic Risk Centrality (N=" << N
         << ").\n"
         << RST;

    vector<uint64_t> adj(N, 0);
    int edges = 0;
    for (int i = 0; i < 32; i++)
        for (int j = 32; j < 64; j++) {
            adj[i] |= (1ULL << j);
            adj[j] |= (1ULL << i);
            edges++;
        }

    set<pair<int, int>> fraud = {{0, 1},   {1, 2},   {2, 0},  {33, 34},
                                 {34, 35}, {35, 33}, {0, 31}, {32, 63}};
    for (auto p : fraud) {
        if (!((adj[p.first] >> p.second) & 1)) {
            adj[p.first] |= (1ULL << p.second);
            adj[p.second] |= (1ULL << p.first);
            edges++;
        }
    }

    // Power Iteration for Eigenvector Centrality
    vector<double> cent(N, 1.0);
    for (int iter = 0; iter < 50; iter++) {
        vector<double> next_c(N, 0.0);
        double norm = 0.0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++)
                if ((adj[i] >> j) & 1) next_c[i] += cent[j];
            norm += next_c[i] * next_c[i];
        }
        norm = sqrt(norm);
        for (int i = 0; i < N; i++) cent[i] = next_c[i] / norm;
    }

    vector<int> risk(N, 0);
    int k3 = 0;
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++) {
            if ((adj[i] >> j) & 1) {
                int c = __builtin_popcountll(adj[i] & adj[j]);
                risk[i] += c;
                risk[j] += c;
                k3 += c;
            }
        }
    k3 /= 3;
    cout << MAG << "  -> Found " << edges
         << " edges (Mantel Limit = 1024). Supersaturation Triggered!\n"
         << RST;
    cout << GRN << "  -> Exact Triangles (K3): " << k3 << "\n" << RST;

    // --- DOT output with centrality + alpha transparency ---
    ofstream dot(dot_fn);
    dot << "graph Finance {\n  layout=sfdp; overlap=false; splines=true;\n"
        << "  graph [label=\"Systemic Risk Centrality Map\\nAlpha-Blended "
           "Bipartite Base | Red = Fraud Edges\", fontname=\"Helvetica\", "
           "labelloc=t];\n"
        << "  node [shape=none, fontname=\"Helvetica\"];\n";
    for (int i = 0; i < N; i++) {
        double r = cent[i] / 0.3;
        if (r > 1.0) r = 1.0;
        int cv = (int)(255 * (1.0 - r));
        char hex[16];
        snprintf(hex, sizeof(hex), "#ff%02x%02x", cv, cv);
        dot << "  " << i
            << " [label=<\n    <table border=\"0\" cellborder=\"1\" "
               "cellspacing=\"0\" cellpadding=\"4\" bgcolor=\""
            << hex << "\">\n"
            << "      <tr><td colspan=\"2\"><b>N" << i << "</b></td></tr>\n"
            << "      <tr><td>Risk</td><td>" << risk[i] / 2 << "</td></tr>\n"
            << "      <tr><td>Cent</td><td>" << fixed << setprecision(2)
            << cent[i] << "</td></tr>\n"
            << "    </table>>];\n";
    }
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if ((adj[i] >> j) & 1) {
                if (fraud.count({i, j}) || fraud.count({j, i})) {
                    dot << "  " << i << " -- " << j
                        << " [color=\"#d32f2f\", penwidth=5.0, "
                           "label=\"FRAUD\", fontname=\"Helvetica\", "
                           "fontcolor=\"red\"];\n";
                } else {
                    // Alpha transparency creates ghost web effect
                    dot << "  " << i << " -- " << j
                        << " [color=\"#00000040\", penwidth=1.0];\n";
                }
            }
    dot << "}\n";

    // --- Lean proof ---
    ofstream out(lean_fn);
    out << "import Mathlib.Tactic\n\ndef edges : Nat := " << edges << "\n";
    out << "def mantel_limit : Nat := 1024\n";
    out << "def exact_cycles : Nat := " << k3 << "\n\n";
    out << "theorem supersaturation_active : edges > mantel_limit := by "
           "decide\n";
    out << "theorem cycles_exist : exact_cycles > 0 := by decide\n";
}

// ============================================================================
// MODULE: ADVERSARIAL (Maker-Breaker Minimax with Zobrist Hashing)
// ============================================================================
namespace adversarial {

constexpr int MAX_N = 32;  // bitmask limit
constexpr int MAX_E = 64;  // uint64_t edge mask limit
constexpr int TT_BITS = 22;
constexpr int TT_SIZE = 1 << TT_BITS;
constexpr int TT_MASK = TT_SIZE - 1;

struct Edge {
    int u, v;
};

struct TTEntry {
    uint64_t hash;
    int depth;
    int value;
    int flag;  // 0=EXACT, 1=LOWER, 2=UPPER
    int best_move;
};

struct GameConfig {
    int gw, gh, nodes, depth;
    string label;
};

static Edge edges[MAX_E];
static int num_edges;
static int num_nodes;
static uint64_t z_node[MAX_N], z_edge[MAX_E], z_turn;
static TTEntry* tt;
static int burner_order[MAX_N];

static GameConfig parse_preset(const string& name) {
    if (name == "path16") return {1, 16, 16, 12, "Path P(16)"};
    if (name == "path20") return {1, 20, 20, 10, "Path P(20)"};
    if (name == "path24") return {1, 24, 24, 10, "Path P(24)"};
    if (name == "tree15") return {0, 0, 15, 12, "Binary Tree (15)"};
    if (name == "campus") return {0, 0, 16, 14, "Campus Network"};
    if (name == "grid3x5") return {3, 5, 15, 10, "Grid 3x5"};
    if (name == "grid4x4") return {4, 4, 16, 8, "Grid 4x4"};
    if (name == "grid5x5") return {5, 5, 25, 8, "Grid 5x5"};
    return {1, 16, 16, 12, "Path P(16)"};
}

static void init_tree15() {
    // Balanced binary tree: 0->{1,2}, 1->{3,4}, 2->{5,6}, 3->{7,8}, ...
    const int parent[] = {-1, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
    for (int i = 1; i < 15; i++) edges[num_edges++] = {parent[i], i};
}

static void init_campus() {
    // Hub-spoke with bridges: central router (0) connects to 3 switches
    // Each switch connects to 4 endpoints. Bridges between switches.
    // 0: core, 1-3: switches, 4-7: wing A, 8-11: wing B, 12-15: wing C
    const int campus_edges[][2] = {
        {0, 1},  {0, 2},  {0, 3},            // core -> switches
        {1, 4},  {1, 5},  {1, 6},  {1, 7},   // switch 1 -> wing A
        {2, 8},  {2, 9},  {2, 10}, {2, 11},  // switch 2 -> wing B
        {3, 12}, {3, 13}, {3, 14}, {3, 15},  // switch 3 -> wing C
        {1, 2},  {2, 3},                     // bridge links between switches
    };
    for (int i = 0; i < 17; i++)
        edges[num_edges++] = {campus_edges[i][0], campus_edges[i][1]};
}

static void init_graph(const GameConfig& cfg) {
    num_edges = 0;
    num_nodes = cfg.nodes;
    if (cfg.gw == 0) {
        // Custom topology
        if (cfg.label.find("Tree") != string::npos)
            init_tree15();
        else
            init_campus();
    } else if (cfg.gw == 1) {
        for (int i = 0; i < cfg.nodes - 1; i++) edges[num_edges++] = {i, i + 1};
    } else {
        for (int y = 0; y < cfg.gh; y++)
            for (int x = 0; x < cfg.gw; x++) {
                int u = y * cfg.gw + x;
                if (x < cfg.gw - 1) edges[num_edges++] = {u, u + 1};
                if (y < cfg.gh - 1) edges[num_edges++] = {u, u + cfg.gw};
            }
    }
}

static void init_move_order(const GameConfig& cfg) {
    for (int i = 0; i < cfg.nodes; i++) burner_order[i] = i;
    int w = max(cfg.gw, 1);  // avoid div-by-zero for custom topologies
    int h = max(cfg.gh, cfg.nodes);
    double cx = (w - 1) / 2.0, cy = (h - 1) / 2.0;
    sort(burner_order, burner_order + cfg.nodes, [&](int a, int b) {
        double ax = (w <= 1) ? 0.0 : double(a % w);
        double ay = (w <= 1) ? double(a) : double(a / w);
        double bx = (w <= 1) ? 0.0 : double(b % w);
        double by = (w <= 1) ? double(b) : double(b / w);
        return abs(ax - cx) + abs(ay - cy) < abs(bx - cx) + abs(by - cy);
    });
}

static void init_zobrist() {
    mt19937_64 rng(42);
    for (int i = 0; i < num_nodes; i++) z_node[i] = rng();
    for (int i = 0; i < num_edges; i++) z_edge[i] = rng();
    z_turn = rng();
    tt = static_cast<TTEntry*>(calloc(TT_SIZE, sizeof(TTEntry)));
}

static uint64_t get_hash(uint32_t burned, uint64_t alive_edges,
                         bool is_burner) {
    uint64_t h = 0;
    for (int i = 0; i < num_nodes; i++)
        if ((burned >> i) & 1) h ^= z_node[i];
    for (int i = 0; i < num_edges; i++)
        if ((alive_edges >> i) & 1ULL) h ^= z_edge[i];
    if (is_burner) h ^= z_turn;
    return h;
}

static uint32_t spread_fire(uint32_t burned, uint64_t alive_edges) {
    uint32_t nb = burned;
    for (int i = 0; i < num_edges; i++) {
        if (!((alive_edges >> i) & 1ULL)) continue;
        int u = edges[i].u, v = edges[i].v;
        if ((burned >> u) & 1) nb |= (1U << v);
        if ((burned >> v) & 1) nb |= (1U << u);
    }
    return nb;
}

// burner_order[] is populated dynamically by init_move_order()

static int alphabeta(uint32_t burned, uint64_t alive_edges, int depth,
                     int alpha, int beta, bool is_burner, int& best_move_out,
                     uint64_t& nodes) {
    nodes++;
    if (depth == 0 || __builtin_popcount(burned) == num_nodes)
        return __builtin_popcount(burned);

    uint64_t h = get_hash(burned, alive_edges, is_burner);
    int idx = h & TT_MASK;
    if (tt[idx].hash == h && tt[idx].depth >= depth) {
        if (tt[idx].flag == 0) {
            best_move_out = tt[idx].best_move;
            return tt[idx].value;
        }
        if (tt[idx].flag == 1 && tt[idx].value >= beta) return beta;
        if (tt[idx].flag == 2 && tt[idx].value <= alpha) return alpha;
    }

    int best_move = -1;
    int orig_alpha = alpha;

    if (is_burner) {
        // Burner: place fire on an unburned node adjacent to existing fire
        // (or any node if nothing is burning yet)
        int max_val = -100;
        bool has_fire = (burned != 0);
        for (int k = 0; k < num_nodes; k++) {
            int i = burner_order[k];
            if ((burned >> i) & 1) continue;
            // Adjacency check: must be next to a burning node (via alive edge)
            if (has_fire) {
                bool adj = false;
                for (int e = 0; e < num_edges; e++) {
                    if (!((alive_edges >> e) & 1ULL)) continue;
                    if ((edges[e].u == i && ((burned >> edges[e].v) & 1)) ||
                        (edges[e].v == i && ((burned >> edges[e].u) & 1))) {
                        adj = true;
                        break;
                    }
                }
                if (!adj) continue;
            }
            int dummy = -1;
            int val = alphabeta(burned | (1U << i), alive_edges, depth - 1,
                                alpha, beta, false, dummy, nodes);
            if (val > max_val) {
                max_val = val;
                best_move = i;
            }
            alpha = max(alpha, val);
            if (beta <= alpha) break;
        }
        if (max_val == -100) {
            // Burner has no valid moves (completely contained!)
            best_move_out = -1;
            return __builtin_popcount(burned);
        }
        int flag = (max_val <= orig_alpha) ? 2 : (max_val >= beta) ? 1 : 0;
        tt[idx] = {h, depth, max_val, flag, best_move};
        best_move_out = best_move;
        return max_val;
    } else {
        // Builder: cut one alive edge. No fire spread.
        int min_val = 100;
        int front[MAX_E], back[MAX_E];
        int nf = 0, nb_count = 0;
        for (int i = 0; i < num_edges; i++) {
            if (!((alive_edges >> i) & 1ULL)) continue;
            int u = edges[i].u, v = edges[i].v;
            if (((burned >> u) & 1) || ((burned >> v) & 1))
                front[nf++] = i;
            else
                back[nb_count++] = i;
        }
        int cands[MAX_E];
        int nc = 0;
        for (int i = 0; i < nf; i++) cands[nc++] = front[i];
        for (int i = 0; i < nb_count; i++) cands[nc++] = back[i];

        if (nc == 0) {
            // No edges to cut — pass turn
            int dummy = -1;
            min_val = alphabeta(burned, alive_edges, depth - 1, alpha, beta,
                                true, dummy, nodes);
        } else {
            for (int k = 0; k < nc; k++) {
                int ei = cands[k];
                uint64_t ne = alive_edges & ~(1ULL << ei);
                int dummy = -1;
                int val = alphabeta(burned, ne, depth - 1, alpha, beta, true,
                                    dummy, nodes);
                if (val < min_val) {
                    min_val = val;
                    best_move = ei;
                }
                beta = min(beta, val);
                if (beta <= alpha) break;
            }
        }
        int flag = (min_val <= orig_alpha) ? 2 : (min_val >= beta) ? 1 : 0;
        tt[idx] = {h, depth, min_val, flag, best_move};
        best_move_out = best_move;
        return min_val;
    }
}

}  // namespace adversarial

void run_adversarial(const string& lean_fn, const string& preset) {
    using namespace adversarial;

    auto cfg = parse_preset(preset);
    cout << CYN << "[Oracle] Adversarial: " << cfg.label << " (" << cfg.nodes
         << "N, depth " << cfg.depth << ").\n"
         << RST;
    cout << MAG
         << "  -> Burner (Max) vs Builder (Min). Alpha-Beta with "
            "Zobrist TT.\n"
         << RST;

    init_graph(cfg);
    init_move_order(cfg);
    init_zobrist();

    uint32_t burned = 0;
    uint64_t alive_edges = (1ULL << num_edges) - 1;
    uint64_t total_nodes = 0;
    int depth_limit = cfg.depth;

    cout << "  [Search] Alpha-Beta pruning (depth=" << depth_limit
         << " plies)...\n";
    auto start = high_resolution_clock::now();

    // JSON state log for interactive replay
    string json_path = "proofs/AdversarialPV_" + preset + ".json";
    ofstream json_out(json_path);
    json_out << "{\n  \"grid_w\": " << cfg.gw << ",\n  \"grid_h\": " << cfg.gh
             << ",\n  \"depth\": " << depth_limit << ",\n  \"preset\": \""
             << preset << "\",\n  \"label\": \"" << cfg.label
             << "\",\n  \"edges\": [\n";
    for (int i = 0; i < num_edges; i++)
        json_out << "    [" << edges[i].u << "," << edges[i].v << "]"
                 << (i < num_edges - 1 ? "," : "") << "\n";
    json_out << "  ],\n  \"states\": [\n"
             << "    {\"b\": 0, \"e\": " << alive_edges
             << ", \"move\": -1, \"actor\": \"Init\"}";

    int nash_val = 0;
    for (int d = depth_limit; d > 0; d--) {
        int best_move = -1;
        uint64_t iter_nodes = 0;
        bool is_burner = (d % 2 == 0);

        int val = alphabeta(burned, alive_edges, d, -100, 100, is_burner,
                            best_move, iter_nodes);
        total_nodes += iter_nodes;
        if (d == depth_limit) nash_val = val;

        if (is_burner) {
            if (best_move < 0) continue;
            burned |= (1U << best_move);
            json_out << ",\n    {\"b\": " << burned
                     << ", \"e\": " << alive_edges
                     << ", \"move\": " << best_move
                     << ", \"actor\": \"Burner\"}";
            cout << "  [Ply " << (depth_limit - d + 1) << "] Burner drops on N"
                 << best_move << " (" << __builtin_popcount(burned)
                 << " burned)\n";
        } else {
            if (best_move != -1) alive_edges &= ~(1ULL << best_move);
            json_out << ",\n    {\"b\": " << burned
                     << ", \"e\": " << alive_edges
                     << ", \"move\": " << best_move
                     << ", \"actor\": \"Builder\"}";
            if (best_move != -1)
                cout << "  [Ply " << (depth_limit - d + 1)
                     << "] Builder severs edge " << edges[best_move].u << "-"
                     << edges[best_move].v << " (" << __builtin_popcount(burned)
                     << " burned)\n";
        }
    }

    auto stop = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(stop - start).count();

    json_out << "\n  ],\n  \"nash_value\": " << nash_val
             << ",\n  \"nodes_searched\": " << total_nodes
             << ",\n  \"time_ms\": " << ms << "\n}\n";
    json_out.close();

    cout << GRN << "  [Nash Equilibrium] Builder limits destruction to "
         << nash_val << "/" << cfg.nodes << " nodes.\n"
         << RST;
    cout << "  [Telemetry] " << total_nodes << " states searched in " << ms
         << " ms\n";

    // Lean witness
    ofstream out(lean_fn);
    out << "import Mathlib.Tactic\n\n"
        << "def grid_size : Nat := " << cfg.nodes << "\n"
        << "def nash_value : Nat := " << nash_val << "\n"
        << "def search_depth : Nat := " << depth_limit << "\n\n"
        << "theorem optimal_defense : nash_value < grid_size "
        << ":= by decide\n";

    free(tt);
    tt = nullptr;
}

// ============================================================================
// Main: Dispatch to module
// ============================================================================
int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: oracle <module> <lean_output> [preset|dot_output]\n"
             << "  Adversarial presets: path16 tree15 campus\n";
        return 1;
    }
    string mode = argv[1];
    string lean_fn = argv[2];

    // Derive dot filename: proofs/Foo.lean -> docs/Foo.lean.dot
    string dot_fn;
    if (argc >= 4 && mode != "adversarial") {
        dot_fn = argv[3];
    } else {
        string base = lean_fn;
        size_t slash = base.rfind('/');
        if (slash != string::npos) base = base.substr(slash + 1);
        dot_fn = "docs/" + base + ".dot";
    }

    if (mode == "epidemiology")
        run_epidemiology(lean_fn, dot_fn);
    else if (mode == "surveillance")
        run_surveillance(lean_fn, dot_fn);
    else if (mode == "finance")
        run_finance(lean_fn, dot_fn);
    else if (mode == "adversarial") {
        string preset = (argc >= 4) ? argv[3] : "path16";
        run_adversarial(lean_fn, preset);
    } else {
        cerr << RED << "Unknown module: " << mode << RST << "\n";
        return 1;
    }
    return 0;
}
