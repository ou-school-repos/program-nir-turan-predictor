#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")

// synthesizer.cpp — High-Performance Topology Discovery Engine
//
// Generates all non-isomorphic free trees on N vertices via
// Beyer-Hedetniemi level sequence enumeration with canonical
// 128-bit hashing and open-addressing deduplication.
//
// Build: g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp

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
static bool quiet_mode = false;

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

static uint64_t generate(int n, int top_k, int prune_deg) {
    uint64_t expected = (n <= 30) ? A000055[n] : A000055[30];

    FlatHashSet seen;
    seen.init(expected);
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
            "{\n  \"n\": 0,\n  \"trees_scanned\": 1,\n"
            "  \"path_score\": 1,\n  \"top_k\": []\n}\n");
        fprintf(stderr, "  N=0 | 1 tree | IS=1 (empty set)\n");
        return 1;
    }

    // Max degree constraint for hard pruning (opt-in via --prune [D])
    // Trees with degree > D are never competitive for d3/d4 tracking,
    // so pruning them is safe — but it breaks the A000055 total count.
    const int MAX_ALLOWED_DEG = (prune_deg > 0) ? prune_deg : MAX_N;

    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_last_progress = t_start;

    int L[MAX_N];
    for (int i = 0; i < n; i++) L[i] = i;

    uint64_t unique = 0;
    uint64_t rooted = 0;
    uint64_t pruned = 0;
    uint64_t trees_d3 = 0, trees_d4 = 0;  // A000672, A000602 counters
    uint64_t last_reported = 0;

    while (true) {
        rooted++;

        // Inline parent array + HARD DEGREE PRUNE
        //    Track degree during construction. If any vertex exceeds
        //    MAX_ALLOWED_DEG, record prune_idx and skip everything.
        int prune_idx = -1;
        int deg[MAX_N] = {};
        parent_arr[0] = -1;
        depth_stack[0] = 0;

        for (int i = 1; i < n; i++) {
            int p = depth_stack[L[i] - 1];
            parent_arr[i] = p;
            depth_stack[L[i]] = i;

            deg[i]++;
            deg[p]++;
            if (deg[p] > MAX_ALLOWED_DEG) {
                prune_idx = i;
                pruned++;
                break;
            }
        }

        if (prune_idx == -1) {
            // Compute max degree for this tree
            int max_deg = 0;
            for (int i = 0; i < n; i++)
                if (deg[i] > max_deg) max_deg = deg[i];

            // Build flat adj list (O(N))
            parent_to_adj(n);

            // Canonical hash + center finding (O(N))
            Hash128 h = tree_hash_128(n);

            // Dedup — only score UNIQUE trees
            if (seen.insert(h)) {
                unique++;
                if (max_deg <= 3) trees_d3++;
                if (max_deg <= 4) trees_d4++;

                // Bottom-up DP (O(N))
                for (int i = 0; i < n; i++) {
                    dp_excl[i] = 1;
                    dp_incl[i] = 1;
                }
                for (int i = n - 1; i > 0; i--) {
                    int p = parent_arr[i];
                    dp_excl[p] *= (dp_excl[i] + dp_incl[i]);
                    dp_incl[p] *= dp_excl[i];
                }
                uint64_t score = dp_excl[0] + dp_incl[0];

                // Lazy top-K (invariants only for candidates)
                top_k_add(score, L, n);

                // Leontovich inline sweep (if --leontovich active)
                if (leontovich_mode) {
                    for (auto& tg : leontovich_targets) {
                        if (tg.violation_found) continue;
                        uint64_t hc = tree_hom_target(n, tg.h, tg.adj);
                        if (hc < tg.path_score) {
                            tg.violation_found = true;
                            fprintf(stdout,
                                    "\n*** LEONTOVICH GRAPH FOUND ***\n"
                                    "Target H (graph6) = %s\n"
                                    "Target |V| = %d\n"
                                    "Tree vertices n = %d\n"
                                    "hom(P_%d, H) = %lu\n"
                                    "hom(T_%d, H) = %lu (Violation!)\n"
                                    "Tree edges: ",
                                    tg.g6.c_str(), tg.h, n, n, tg.path_score, n,
                                    hc);
                            for (int j = 1; j < n; j++)
                                fprintf(stdout, "[%d,%d] ", parent_arr[j], j);
                            fprintf(stdout,
                                    "\n********************************\n");
                            fflush(stdout);
                        }
                    }
                    // H-coloring minimizer (if --hcolor active)
                } else if (hcolor_h > 0) {
                    uint64_t hc = tree_hom(n, hcolor_h);
                    hc_sum += hc;
                    hc_podium_add(hc);
                    if (hc <= hc_top[0].score) {
                        hc_best_deg = max_deg;
                        hc_best_n = n;
                        memcpy(hc_best_seq, L, n * sizeof(int));
                    }
                    if (hc < hc_path_score && !hc_violation_found) {
                        hc_violation_found = true;
                        fprintf(stderr,
                                "\n  *** VIOLATION: tree beats path! "
                                "hom(T,H)=%lu < hom(P_%d,H)=%lu ***\n",
                                hc, n, hc_path_score);
                    }
                }
            }
        }

        // Progress: update every 100K unique OR every 5s (whichever first)
        {
            auto now = std::chrono::high_resolution_clock::now();
            double ms_since =
                std::chrono::duration<double, std::milli>(now - t_last_progress)
                    .count();
            bool count_trigger = (unique >= last_reported + 100000);
            bool time_trigger = (ms_since >= 5000.0 && unique > last_reported);

            if (count_trigger || time_trigger) {
                double ms_total =
                    std::chrono::duration<double, std::milli>(now - t_start)
                        .count();
                double inst_rate =
                    (unique - last_reported) / (ms_since / 1000.0);
                last_reported = unique;
                t_last_progress = now;

                uint64_t target = (n <= 30) ? A000055[n] : 0;
                if (target > 0 && prune_deg == 0) {
                    // Unpruned: show accurate progress vs A000055
                    double pct = 100.0 * unique / target;
                    double eta_s = (target - unique) / inst_rate;
                    fprintf(stderr,
                            "\r  [c++] %luK / %luK (%.0f%%) | %.1fs | "
                            "%.0fK/s | ETA %.0fs | pruned %.1fM   ",
                            unique / 1000, target / 1000, pct,
                            ms_total / 1000.0, inst_rate / 1000.0, eta_s,
                            pruned / 1e6);
                } else if (target > 0) {
                    // Pruned: show found count / total + skipped
                    fprintf(stderr,
                            "\r  [c++] %luK / %luK found | %.1fs | "
                            "%.0fK/s | skipped %.1fM   ",
                            unique / 1000, target / 1000, ms_total / 1000.0,
                            inst_rate / 1000.0, pruned / 1e6);
                } else {
                    fprintf(
                        stderr, "\r  [c++] %luK trees | %.1fs | %.0fK/s    ",
                        unique / 1000, ms_total / 1000.0, inst_rate / 1000.0);
                }
            }
        }

        // Beyer-Hedetniemi successor (amortized O(1))
        // KEY: if we pruned, backtrack from prune_idx instead of n-1
        int p = (prune_idx != -1) ? prune_idx : n - 1;
        while (p > 0 && L[p] <= 1) p--;
        if (p == 0) break;

        int q = p - 1;
        while (L[q] != L[p] - 1) q--;

        int period = p - q;
        for (int i = p; i < n; i++) L[i] = L[i - period];
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    // End-of-run summary
    double dedup_ratio = (double)rooted / unique;
    double load = (double)seen.size() / seen.cap;
    // Unconstrained max is always the star: 2^{N-1}+1 (analytically known)
    uint64_t star_score = (1ULL << (n - 1)) + 1;
    if (best_any.score < star_score) best_any.score = star_score;
    uint64_t top1 = best_any.score;
    uint64_t p_sc = path_score(n);
    fprintf(stderr,
            "\n  ═══════════════════════════════════════════════\n"
            "  N=%d | %lu unique / %lu rooted | %.1fs\n"
            "  Throughput: %.0f trees/sec | Dedup ratio: %.1fx\n"
            "  Hash load: %.1f%% (%lu / %lu slots)\n"
            "  Trees: %lu (d≤3: %lu, d≤4: %lu)\n"
            "  Top-1 (any): %lu (%.2fx vs path)\n"
            "  Top-1 (d≤3): %lu | Top-1 (d≤4): %lu\n"
            "  ═══════════════════════════════════════════════\n",
            n, unique, rooted, elapsed_ms / 1000.0,
            unique / (elapsed_ms / 1000.0), dedup_ratio, load * 100.0,
            seen.size(), seen.cap, unique, trees_d3, trees_d4, top1,
            (double)top1 / p_sc, best_d3.score, best_d4.score);

    // H-coloring summary
    if (hcolor_h > 0) {
        uint64_t sum_hi = (uint64_t)(hc_sum >> 64);
        uint64_t sum_lo = (uint64_t)hc_sum;
        fprintf(stderr,
                "  ── H-coloring (P_%d target) ──\n"
                "  hom(P_%d, P_%d) = %lu (path baseline)\n"
                "  hom(K_1_%d, P_%d) = %lu (star)\n",
                hcolor_h, n, hcolor_h, hc_path_score, n - 1, hcolor_h,
                hc_star_score);
        uint64_t rank = 1;
        for (int i = 0; i < HC_PODIUM && hc_top[i].tie_count > 0; i++) {
            fprintf(stderr, "  #%-3lu hom(T, P_%d) = %lu (%lu trees)\n", rank,
                    hcolor_h, hc_top[i].score, hc_top[i].tie_count);
            rank += hc_top[i].tie_count;
        }
        fprintf(stderr, "  sum hom(T, P_%d)  = ", hcolor_h);
        if (sum_hi > 0)
            fprintf(stderr, "%lu%019lu", sum_hi, sum_lo);
        else
            fprintf(stderr, "%lu", sum_lo);
        fprintf(stderr,
                " (checksum)\n"
                "  trees: %lu | ratio: %.6f\n"
                "  Path is %s\n"
                "  ═══════════════════════════════════════════════\n",
                unique,
                hc_top[0].tie_count > 0
                    ? (double)hc_top[0].score / hc_path_score
                    : 0.0,
                hc_violation_found ? "NOT MINIMAL — violation found!"
                                   : "minimal (no violation)");
    }

    // Auto-log to docs/runs/benchmark.log
    {
        system("mkdir -p docs/runs");
        FILE* log = fopen("docs/runs/benchmark.log", "a");
        if (log) {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", localtime(&t));
            fprintf(log,
                    "%s | N=%-2d | %7lu trees | %8lu rooted | %5.1fs | "
                    "%7.0f/s | dedup %5.1fx | load %5.1f%% | top1 %7lu"
                    " | d3 %7lu | d4 %7lu\n",
                    ts, n, unique, rooted, elapsed_ms / 1000.0,
                    unique / (elapsed_ms / 1000.0), dedup_ratio, load * 100.0,
                    top1, best_d3.score, best_d4.score);
            fclose(log);
        }
    }

    // Auto-append JSONL to docs/runs/sequence.jsonl (one line per N)
    {
        FILE* jl = fopen("docs/runs/sequence.jsonl", "a");
        if (jl) {
            char line[4096];
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", localtime(&t));

            int par[MAX_N], ds_arr[MAX_N];
            par[0] = -1;
            ds_arr[0] = 0;
            if (best_d3.n > 0) {
                for (int i = 1; i < best_d3.n; i++) {
                    par[i] = ds_arr[best_d3.level_seq[i] - 1];
                    ds_arr[best_d3.level_seq[i]] = i;
                }
            }

            // Build edge list string
            char edges_buf[2048] = {};
            int pos = 0;
            for (int i = 1; i < best_d3.n; i++) {
                if (i > 1)
                    pos +=
                        snprintf(edges_buf + pos, sizeof(edges_buf) - pos, ",");
                pos += snprintf(edges_buf + pos, sizeof(edges_buf) - pos,
                                "[%d,%d]", par[i], i);
            }

            // Build H-coloring minimizer edges if active
            char hc_edges_buf[2048] = {};
            if (hcolor_h > 0 && hc_best_n > 0) {
                int hpar[MAX_N], hds[MAX_N];
                hpar[0] = -1;
                hds[0] = 0;
                for (int i = 1; i < hc_best_n; i++) {
                    hpar[i] = hds[hc_best_seq[i] - 1];
                    hds[hc_best_seq[i]] = i;
                }
                int hp = 0;
                for (int i = 1; i < hc_best_n; i++) {
                    if (i > 1)
                        hp += snprintf(hc_edges_buf + hp,
                                       sizeof(hc_edges_buf) - hp, ",");
                    hp += snprintf(hc_edges_buf + hp, sizeof(hc_edges_buf) - hp,
                                   "[%d,%d]", hpar[i], i);
                }
            }

            if (hcolor_h > 0) {
                snprintf(line, sizeof(line),
                         "{\"ts\":\"%s\",\"n\":%d,\"trees\":%lu,"
                         "\"trees_d3\":%lu,\"trees_d4\":%lu,"
                         "\"rooted\":%lu,\"pruned\":%lu,"
                         "\"path\":%lu,\"d3\":%lu,\"d4\":%lu,\"any\":%lu,"
                         "\"d3_deg\":%d,\"d3_leaves\":%d,\"d3_diam\":%d,"
                         "\"d3_edges\":[%s],"
                         "\"hc_target\":\"P%d\",\"hc_path\":%lu,"
                         "\"hc_min\":%lu,\"hc_min_ties\":%lu,"
                         "\"hc_2nd\":%lu,\"hc_2nd_ties\":%lu,"
                         "\"hc_3rd\":%lu,\"hc_3rd_ties\":%lu,"
                         "\"hc_ratio\":%.6f,"
                         "\"hc_violation\":%s,"
                         "\"hc_min_deg\":%d,\"hc_min_edges\":[%s],"
                         "\"ms\":%.0f}\n",
                         ts, n, unique, trees_d3, trees_d4, rooted, pruned,
                         p_sc, best_d3.score, best_d4.score, best_any.score,
                         best_d3.max_degree, best_d3.leaves, best_d3.diameter,
                         edges_buf, hcolor_h, hc_path_score, hc_top[0].score,
                         hc_top[0].tie_count, hc_top[1].score,
                         hc_top[1].tie_count, hc_top[2].score,
                         hc_top[2].tie_count,
                         hc_top[0].tie_count > 0
                             ? (double)hc_top[0].score / hc_path_score
                             : 0.0,
                         hc_violation_found ? "true" : "false", hc_best_deg,
                         hc_edges_buf, elapsed_ms);
            } else {
                snprintf(line, sizeof(line),
                         "{\"ts\":\"%s\",\"n\":%d,\"trees\":%lu,"
                         "\"trees_d3\":%lu,\"trees_d4\":%lu,"
                         "\"rooted\":%lu,\"pruned\":%lu,"
                         "\"path\":%lu,\"d3\":%lu,\"d4\":%lu,\"any\":%lu,"
                         "\"d3_deg\":%d,\"d3_leaves\":%d,\"d3_diam\":%d,"
                         "\"d3_edges\":[%s],\"ms\":%.0f}\n",
                         ts, n, unique, trees_d3, trees_d4, rooted, pruned,
                         p_sc, best_d3.score, best_d4.score, best_any.score,
                         best_d3.max_degree, best_d3.leaves, best_d3.diameter,
                         edges_buf, elapsed_ms);
            }
            fputs(line, jl);
            fclose(jl);
        }
    }

    // JSON output with constrained extremals
    auto print_tree = [&](const ConstrainedBest& t, const char* label) {
        int par[MAX_N], ds[MAX_N];
        par[0] = -1;
        ds[0] = 0;
        for (int i = 1; i < t.n; i++) {
            par[i] = ds[t.level_seq[i] - 1];
            ds[t.level_seq[i]] = i;
        }
        printf("    {\n");
        printf("      \"constraint\": \"%s\",\n", label);
        printf("      \"score\": %lu,\n", t.score);
        printf("      \"ratio\": %.2f,\n", (double)t.score / p_sc);
        printf("      \"max_degree\": %d,\n", t.max_degree);
        printf("      \"diameter\": %d,\n", t.diameter);
        printf("      \"leaves\": %d,\n", t.leaves);
        printf("      \"edges\": [");
        bool first = true;
        for (int i = 1; i < t.n; i++) {
            if (!first) printf(", ");
            printf("[%d,%d]", par[i], i);
            first = false;
        }
        printf("]\n    }");
    };

    printf("{\n");
    printf("  \"n\": %d,\n", n);
    printf("  \"trees_scanned\": %lu,\n", unique);
    printf("  \"rooted_processed\": %lu,\n", rooted);
    printf("  \"path_score\": %lu,\n", p_sc);
    printf("  \"elapsed_ms\": %.1f,\n", elapsed_ms);
    printf("  \"trees_per_sec\": %.0f,\n", unique / (elapsed_ms / 1000.0));
    printf("  \"top_k\": [\n");
    if (best_d3.score > 0) {
        print_tree(best_d3, "Delta <= 3 (Chemical / 6G Routing)");
        printf(",\n");
    }
    if (best_d4.score > 0) {
        print_tree(best_d4, "Delta <= 4 (Telecom Hubs)");
        printf(",\n");
    }
    print_tree(best_any, "Unconstrained");
    printf("\n  ]\n}\n");

    return unique;
}

// =====================================================================
// Main
// =====================================================================
int main(int argc, const char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        fprintf(
            stderr,
            "Usage: %s N [--top K] [--prune [D]] [--hcolor PK]\n"
            "           [--hgraph G6] [--leontovich K] [--quiet]\n"
            "\n"
            "  N              Number of vertices (0..%d)\n"
            "  --top K        Track top-K extremal trees (default: 10)\n"
            "  --prune [D]    Hard-prune trees with degree > D (default D=4)\n"
            "  --hcolor PK    Find tree minimizing hom(T, P_K)\n"
            "  --hgraph G6    Test single target H in graph6 format\n"
            "  --leontovich K Run geng to test ALL connected H on K vertices\n"
            "  --quiet        Suppress progress/JSON output\n"
            "\n"
            "Output: JSON to stdout, telemetry to stderr,\n"
            "        auto-appends to docs/runs/sequence.jsonl\n",
            argv[0], MAX_N);
        return 1;
    }

    int n = -1;
    int top_k = 10;
    int prune_deg = 0;  // 0 = no pruning
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            top_k = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet_mode = true;
        } else if (strcmp(argv[i], "--prune") == 0) {
            // --prune alone defaults to 4, --prune 3 prunes at degree 3
            if (i + 1 < argc && argv[i + 1][0] >= '2' && argv[i + 1][0] <= '9')
                prune_deg = atoi(argv[++i]);
            else
                prune_deg = 4;
        } else if (strcmp(argv[i], "--hcolor") == 0 && i + 1 < argc) {
            i++;
            // Parse "P5", "P7", etc.
            if (argv[i][0] == 'P' || argv[i][0] == 'p') {
                int k = atoi(argv[i] + 1);
                if (k >= 2 && k <= 30) {
                    build_path_target(k);
                    fprintf(stderr, "  H-coloring mode: target = P_%d\n", k);
                } else {
                    fprintf(stderr, "Invalid path size: %s\n", argv[i]);
                    return 1;
                }
            } else {
                fprintf(stderr, "Unknown target: %s (use P5, P7, etc.)\n",
                        argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--hgraph") == 0 && i + 1 < argc) {
            i++;
            TargetGraph tg;
            tg.g6 = argv[i];
            parse_graph6(tg.g6.c_str(), tg.h, tg.adj);
            tg.violation_found = false;
            leontovich_targets.push_back(tg);
            leontovich_mode = true;
        } else if (strcmp(argv[i], "--leontovich") == 0 && i + 1 < argc) {
            i++;
            int k = atoi(argv[i]);
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "geng -c %d -q 2>/dev/null", k);
            FILE* pipe = popen(cmd, "r");
            if (!pipe) {
                fprintf(stderr, "Error: failed to run geng\n");
                return 1;
            }
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) {
                buf[strcspn(buf, "\r\n")] = 0;
                if (strlen(buf) > 0) {
                    TargetGraph tg;
                    tg.g6 = buf;
                    parse_graph6(buf, tg.h, tg.adj);
                    tg.violation_found = false;
                    leontovich_targets.push_back(tg);
                }
            }
            pclose(pipe);
            leontovich_mode = true;
            fprintf(stderr, "  Loaded %zu connected graphs on %d vertices.\n",
                    leontovich_targets.size(), k);
        } else if (argv[i][0] != '-' && n < 0) {
            n = atoi(argv[i]);
        }
    }

    if (n < 0 || n > MAX_N) {
        fprintf(stderr, "N must be in [0, %d]\n", MAX_N);
        return 1;
    }

    fprintf(
        stderr, "[c++ synthesizer] Enumerating trees on N=%d (top_k=%d%s)\n", n,
        top_k,
        prune_deg > 0
            ? (std::string(", PRUNING d>") + std::to_string(prune_deg)).c_str()
            : "");

    uint64_t p_sc = path_score(n);
    fprintf(stderr, "  Path P(%d) baseline: %lu independent sets\n", n, p_sc);

    // Precompute H-coloring path baseline
    if (hcolor_h > 0) {
        hc_path_score = path_hom(n, hcolor_h);
        fprintf(stderr, "  hom(P_%d, P_%d) baseline: %lu\n", n, hcolor_h,
                hc_path_score);
    }

    generate(n, top_k, prune_deg);

    // Report Leontovich summary
    if (leontovich_mode) {
        int violations = 0;
        for (const auto& tg : leontovich_targets)
            if (tg.violation_found) violations++;
        fprintf(
            stderr,
            "\n  Leontovich sweep: %d / %zu graphs are Leontovich at n=%d\n",
            violations, leontovich_targets.size(), n);
    }
    return 0;
}
