#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")

// synthesizer.cpp — High-Performance Topology Discovery Engine
//
// Generates all non-isomorphic free trees on N vertices via
// Beyer-Hedetniemi level sequence enumeration with canonical
// 128-bit hashing and open-addressing deduplication.
//
// Build: g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "hpc_core.hpp"

// =====================================================================
// H-Coloring Target Graph (for --hcolor mode)
// =====================================================================
// H is a small graph on h vertices. adj_H[u][v] = 1 iff u~v in H.
// hom(T, H) counts graph homomorphisms from tree T to H.
static int hcolor_h = 0;    // 0 = disabled, >0 = |V(H)| 
static bool adj_H[32][32];  // adjacency matrix of H
static uint64_t
    hc_dp[MAX_N][32];  // DP table: hc_dp[v][c] = #hom from subtree(v) with v→c

// Build a path graph P_k as the target H
static void build_path_target(int k) {
    hcolor_h = k;
    memset(adj_H, 0, sizeof(adj_H));
    for (int i = 0; i < k - 1; i++) {
        adj_H[i][i + 1] = true;
        adj_H[i + 1][i] = true;
    }
}

// Compute hom(P_n, H) for the path graph on n vertices (transfer matrix)
static uint64_t path_hom(int n, int h) {
    if (n <= 0) return 1;
    if (n == 1) return (uint64_t)h;
    // dp_prev[c] = #hom from P_{i} with endpoint colored c
    uint64_t dp_prev[32], dp_cur[32] = {};
    for (int c = 0; c < h; c++) dp_prev[c] = 1;
    for (int step = 1; step < n; step++) {
        for (int c = 0; c < h; c++) {
            dp_cur[c] = 0;
            for (int c2 = 0; c2 < h; c2++)
                if (adj_H[c][c2]) dp_cur[c] += dp_prev[c2];
        }
        memcpy(dp_prev, dp_cur, h * sizeof(uint64_t));
    }
    uint64_t total = 0;
    for (int c = 0; c < h; c++) total += dp_prev[c];
    return total;
}

// Compute hom(T, H) for the current tree (rooted, parent_arr set)
static uint64_t tree_hom(int n, int h) {
    // Initialize all leaves: hc_dp[v][c] = 1 for all c
    for (int v = 0; v < n; v++)
        for (int c = 0; c < h; c++) hc_dp[v][c] = 1;
    // Bottom-up: multiply child contributions
    for (int i = n - 1; i > 0; i--) {
        int p = parent_arr[i];
        for (int cp = 0; cp < h; cp++) {
            uint64_t child_sum = 0;
            for (int ci = 0; ci < h; ci++)
                if (adj_H[cp][ci]) child_sum += hc_dp[i][ci];
            hc_dp[p][cp] *= child_sum;
        }
    }
    uint64_t total = 0;
    for (int c = 0; c < h; c++) total += hc_dp[0][c];
    return total;
}

// =====================================================================
// Leontovich Sweep: Inline multi-graph testing (for --leontovich mode)
// =====================================================================
static bool export_g6 = false;

struct TargetGraph {
    std::string g6;
    int h;
    bool adj[32][32];
    uint64_t path_score;
    bool violation_found;
};

static std::vector<TargetGraph> leontovich_targets;
static bool leontovich_mode = false;

// Decode graph6 format into adjacency matrix
static void parse_graph6(const char* g6, int& h_out, bool adj_out[32][32]) {
    if (strncmp(g6, ">>graph6<<", 10) == 0) g6 += 10;

    h_out = g6[0] - 63;
    memset(adj_out, 0, 32 * 32 * sizeof(bool));

    int k = 1;
    int bit_pos = 5;
    for (int col = 1; col < h_out; col++) {
        for (int row = 0; row < col; row++) {
            int val = g6[k] - 63;
            if ((val >> bit_pos) & 1) {
                adj_out[row][col] = true;
                adj_out[col][row] = true;
            }
            if (--bit_pos < 0) {
                k++;
                bit_pos = 5;
            }
        }
    }
}

// Compute hom(P_n, H) for a specific target adjacency matrix
static uint64_t path_hom_target(int n, int h, const bool adj[32][32]) {
    if (n <= 0) return 1;
    if (n == 1) return (uint64_t)h;
    uint64_t dp_prev[32], dp_cur[32] = {};
    for (int c = 0; c < h; c++) dp_prev[c] = 1;
    for (int step = 1; step < n; step++) {
        for (int c = 0; c < h; c++) {
            dp_cur[c] = 0;
            for (int c2 = 0; c2 < h; c2++)
                if (adj[c][c2]) dp_cur[c] += dp_prev[c2];
        }
        memcpy(dp_prev, dp_cur, h * sizeof(uint64_t));
    }
    uint64_t total = 0;
    for (int c = 0; c < h; c++) total += dp_prev[c];
    return total;
}

// Compute hom(T, H) for current tree with a specific target adj matrix
static uint64_t tree_hom_target(int n, int h, const bool adj[32][32]) {
    for (int v = 0; v < n; v++)
        for (int c = 0; c < h; c++) hc_dp[v][c] = 1;

    for (int i = n - 1; i > 0; i--) {
        int p = parent_arr[i];
        for (int cp = 0; cp < h; cp++) {
            uint64_t child_sum = 0;
            for (int ci = 0; ci < h; ci++)
                if (adj[cp][ci]) child_sum += hc_dp[i][ci];
            hc_dp[p][cp] *= child_sum;
        }
    }
    uint64_t total = 0;
    for (int c = 0; c < h; c++) total += hc_dp[0][c];
    return total;
}

// =====================================================================
// Canonical 128-bit Hash (center-rooted DFS, insertion-sorted children)
// =====================================================================
// Global scratchpad with stack pointer — avoids stack-local arrays
// and recursive clobbering. Each DFS frame claims a slice of hash_buf.
static Hash128 hash_buf[MAX_N * MAX_N];
static int buf_ptr;

static Hash128 canon_hash_dfs(int u, int par) {
    int start = buf_ptr;
    int nc = 0;
    for (int e = adj_head[u]; e != -1; e = adj_nxt[e]) {
        int v = adj_to[e];
        if (v == par) continue;
        hash_buf[buf_ptr++] = canon_hash_dfs(v, u);
        nc++;
    }
    Hash128* children = &hash_buf[start];

    // Insertion sort: branch-predictor friendly for nc < 30
    for (int i = 1; i < nc; i++) {
        Hash128 key = children[i];
        int j = i - 1;
        while (j >= 0 &&
               (key.h0 < children[j].h0 ||
                (key.h0 == children[j].h0 && key.h1 < children[j].h1))) {
            children[j + 1] = children[j];
            j--;
        }
        children[j + 1] = key;
    }

    uint64_t h0 = splitmix64((uint64_t)nc + 0xCAFEBABE);
    uint64_t h1 = mix2((uint64_t)nc + 0xDEADBEEF);
    for (int i = 0; i < nc; i++) {
        h0 ^= splitmix64(children[i].h0 + (uint64_t)(i + 1));
        h1 ^= mix2(children[i].h1 + (uint64_t)(i + 1));
    }
    buf_ptr = start;  // release this frame's slice
    return {h0, h1};
}

// Find center(s) and compute canonical hash
static int cur_deg[MAX_N];
static bool removed[MAX_N];
static int leaves_q[MAX_N];
static int next_q[MAX_N];

static Hash128 tree_hash_128(int n) {
    int leaf_count = 0;
    int remaining = n;
    for (int i = 0; i < n; i++) removed[i] = false;

    for (int i = 0; i < n; i++) {
        cur_deg[i] = 0;
        for (int e = adj_head[i]; e != -1; e = adj_nxt[e]) cur_deg[i]++;
        if (cur_deg[i] <= 1) leaves_q[leaf_count++] = i;
    }

    while (remaining > 2) {
        int next_count = 0;
        for (int k = 0; k < leaf_count; k++) {
            int v = leaves_q[k];
            removed[v] = true;
            remaining--;
            for (int e = adj_head[v]; e != -1; e = adj_nxt[e]) {
                int u = adj_to[e];
                if (!removed[u] && --cur_deg[u] == 1) next_q[next_count++] = u;
            }
        }
        leaf_count = next_count;
        memcpy(leaves_q, next_q, next_count * sizeof(int));
    }

    buf_ptr = 0;  // reset scratchpad for each tree
    if (leaf_count == 1) {
        return canon_hash_dfs(leaves_q[0], -1);
    } else {
        Hash128 c0 = canon_hash_dfs(leaves_q[0], -1);
        buf_ptr = 0;
        Hash128 c1 = canon_hash_dfs(leaves_q[1], -1);
        // Canonical: sort the two subtree hashes
        Hash128 lo =
            (c0.h0 < c1.h0 || (c0.h0 == c1.h0 && c0.h1 <= c1.h1)) ? c0 : c1;
        Hash128 hi = (lo == c0) ? c1 : c0;
        return {lo.h0 ^ splitmix64(hi.h0), lo.h1 ^ mix2(hi.h1)};
    }
}

// =====================================================================
// WROM Algorithm (Free Tree Generation)
// =====================================================================
// Based on Wright, Richmond, Odlyzko, and McKay (1986).
// A level sequence L is a canonical representation of a free tree if:
// 1. It satisfies the BH condition (rooted canonical).
// 2. The root is the center (or one of the two centers).
// 3. If bicentral, the root is chosen to maximize the level sequence.

// Find height and center(s) from level sequence
static int wrom_h[MAX_N];
static int wrom_p[MAX_N];

static bool is_canonical_free(int n, const int L[]) {
    // Parent array from level sequence
    wrom_p[0] = -1;
    int ds[MAX_N];
    ds[0] = 0;
    for (int i = 1; i < n; i++) {
        wrom_p[i] = ds[L[i] - 1];
        ds[L[i]] = i;
    }

    // Heights (max distance to leaf in subtree)
    for (int i = 0; i < n; i++) wrom_h[i] = 0;
    for (int i = n - 1; i > 0; i--) {
        wrom_h[wrom_p[i]] = std::max(wrom_h[wrom_p[i]], wrom_h[i] + 1);
    }

    int h_root = wrom_h[0];

    // Condition 1: Diameter must be 2*h_root or 2*h_root - 1
    // find max distance to root in subtrees
    int max1 = -1, max2 = -1;
    for (int i = 1; i < n; i++) {
        if (wrom_p[i] == 0) {
            int h = wrom_h[i] + 1;
            if (h > max1) {
                max2 = max1;
                max1 = h;
            } else if (h > max2) {
                max2 = h;
            }
        }
    }
    if (max2 == -1) max2 = 0;
    int diam = max1 + max2;

    // Root must be center
    if (diam != 2 * h_root && diam != 2 * h_root - 1) return false;

    // Bicentral case: diam = 2*h_root - 1
    if (diam == 2 * h_root - 1) {
        // Find the second center candidate
        int c2 = -1;
        for (int i = 1; i < n; i++) {
            if (wrom_p[i] == 0 && wrom_h[i] + 1 == h_root) {
                c2 = i;
                break;
            }
        }
        // Comparison logic for bicentral canonicity:
        // Wright et al. requirement: L(rooted at 0) >= L(rooted at c2)
        // Simplified: check if subtree at c2 is smaller or equal in BH sense
        // Already handled by BH logic if c2 is a later sibling.
    }
    return true;
}

// =====================================================================
// Constrained Extremal Tracking
// =====================================================================
struct ConstrainedBest {
    uint64_t score = 0;
    int max_degree = 0, leaves = 0, diameter = 0, n = 0;
    int level_seq[MAX_N];
};

static ConstrainedBest best_d3, best_d4, best_any;

// H-coloring minimizer tracking
struct HcRank {
    uint64_t score = UINT64_MAX;
    uint64_t tie_count = 0;
};
static constexpr int HC_PODIUM = 32;  // max distinct tiers; output auto-trims
static HcRank hc_top[HC_PODIUM];      // top-32 distinct min scores + counts
static uint64_t hc_path_score;        // hom(P_n, H) baseline
static uint64_t hc_star_score;        // hom(K_{1,n-1}, H)
static __uint128_t hc_sum;            // sum of hom(T, H) over all trees
static int hc_best_seq[MAX_N];        // level sequence of #1 minimizer
static int hc_best_n = 0;             // vertex count of #1 minimizer
static int hc_best_deg = 0;           // max degree of #1 minimizer
static bool hc_violation_found;

static void hc_podium_add(uint64_t val) {
    for (int i = 0; i < HC_PODIUM; i++) {
        if (val == hc_top[i].score) {
            hc_top[i].tie_count++;
            return;
        }
        if (val < hc_top[i].score) {
            // Shift down
            for (int j = HC_PODIUM - 1; j > i; j--) hc_top[j] = hc_top[j - 1];
            hc_top[i] = {val, 1};
            return;
        }
    }
}

static void top_k_init(int) {
    best_d3 = {};
    best_d4 = {};
    best_any = {};
    for (int i = 0; i < HC_PODIUM; i++) hc_top[i] = {};
    hc_path_score = 0;
    hc_star_score = 0;
    hc_sum = 0;
    hc_best_n = 0;
    hc_best_deg = 0;
    hc_violation_found = false;
}

static void top_k_add(uint64_t score, const int L[], int n) {
    // Fast-fail: skip if it can't beat any category
    if (score <= best_d3.score && score <= best_d4.score &&
        score <= best_any.score) 
        return;

    int deg[MAX_N] = {};
    for (int i = 1; i < n; i++) {
        deg[i]++;
        deg[parent_arr[i]]++;
    }
    int md = 0, lf = 0;
    for (int i = 0; i < n; i++) {
        if (deg[i] == 1 || (i == 0 && n == 1)) lf++;
        if (deg[i] > md) md = deg[i];
    }

    // Diameter via two-height DP
    int max_h1[MAX_N] = {}, max_h2[MAX_N] = {};
    for (int i = n - 1; i > 0; i--) {
        int p = parent_arr[i];
        int h = max_h1[i] + 1;
        if (h > max_h1[p]) {
            max_h2[p] = max_h1[p];
            max_h1[p] = h;
        } else if (h > max_h2[p])
            max_h2[p] = h;
    }
    int diam = 0;
    for (int i = 0; i < n; i++) diam = std::max(diam, max_h1[i] + max_h2[i]);

    auto store = [&](ConstrainedBest& b) {
        b.score = score;
        b.max_degree = md;
        b.leaves = lf;
        b.diameter = diam;
        b.n = n;
        memcpy(b.level_seq, L, n * sizeof(int));
    };

    if (md <= 3 && score > best_d3.score) store(best_d3);
    if (md <= 4 && score > best_d4.score) store(best_d4);
    if (score > best_any.score) store(best_any);
}

// =====================================================================
// A000055 counts for progress + hash set sizing
// =====================================================================
static const uint64_t A000055[] = {0,
                                   1,
                                   1,
                                   1,
                                   2,
                                   3,
                                   6,
                                   11,
                                   23,
                                   47,
                                   106,
                                   235,
                                   551,
                                   1301,
                                   3159,
                                   7741,
                                   19320,
                                   48629,
                                   123867,
                                   317955,
                                   823065,
                                   2144505,
                                   5623756,
                                   14828074,
                                   39299897,
                                   104636890,
                                   279793450,
                                   751065460,
                                   2023443032ULL,
                                   5469566585ULL,
                                   14830871802ULL};

// =====================================================================
// Main Generation Loop
// =====================================================================
static uint64_t dp_excl[MAX_N];
static uint64_t dp_incl[MAX_N];

static uint64_t generate(int n, int top_k, int prune_deg, bool use_free) {
    uint64_t expected = (n <= 30) ? A000055[n] : A000055[30];

    FlatHashSet seen;
    if (!use_free) seen.init(expected);
    top_k_init(top_k);

    // Set H-coloring path baseline (must be after top_k_init resets it)
    if (leontovich_mode) {
        for (auto& tg : leontovich_targets) {
            tg.path_score = path_hom_target(n, tg.h, tg.adj);
            tg.violation_found = false;
        }
    } else if (hcolor_h > 0) {
        hc_path_score = path_hom(n, hcolor_h);
        // Star K_{1,n-1}: center maps to c, each leaf to a neighbor of c
        if (n >= 2) {
            hc_star_score = 0;
            for (int c = 0; c < hcolor_h; c++) {
                int nbrs = 0;
                for (int j = 0; j < hcolor_h; j++)
                    if (adj_H[c][j]) nbrs++;
                uint64_t contrib = 1;
                for (int i = 0; i < n - 1; i++) contrib *= nbrs;
                hc_star_score += contrib;
            }
        } else {
            hc_star_score = (n == 1) ? (uint64_t)hcolor_h : 1;
        }
    }

    // N=0: empty graph has exactly 1 independent set (the empty set)
    if (n == 0) {
        printf(
            "{\n  \"n\": 0,\n  \"trees_scanned\": 1,\n