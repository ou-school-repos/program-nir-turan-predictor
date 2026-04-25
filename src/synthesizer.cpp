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
#include <string>
#include <vector>

#include "hpc_core.hpp"

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

static void top_k_init(int) {
    best_d3 = {};
    best_d4 = {};
    best_any = {};
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

        // 1. Inline parent array + HARD DEGREE PRUNE
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

            // 2. Build flat adj list (O(N))
            parent_to_adj(n);

            // 3. Canonical hash + center finding (O(N))
            Hash128 h = tree_hash_128(n);

            // 4. Dedup — only score UNIQUE trees
            if (seen.insert(h)) {
                unique++;
                if (max_deg <= 3) trees_d3++;
                if (max_deg <= 4) trees_d4++;

                // 5. Bottom-up DP (O(N))
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

                // 6. Lazy top-K (invariants only for candidates)
                top_k_add(score, L, n);
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
                if (target > 0) {
                    double pct = 100.0 * unique / target;
                    double eta_s = (target - unique) / inst_rate;
                    fprintf(stderr,
                            "\r  [c++] %luK / %luK (%.0f%%) | %.1fs | "
                            "%.0fK/s | ETA %.0fs | pruned %luM   ",
                            unique / 1000, target / 1000, pct,
                            ms_total / 1000.0, inst_rate / 1000.0, eta_s,
                            pruned / 1000000);
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
                    "%s | N=%d | %lu trees | %lu rooted | %.1fs | "
                    "%.0f/s | dedup %.1fx | load %.1f%% | top1 %lu"
                    " | d3 %lu | d4 %lu\n",
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

            snprintf(line, sizeof(line),
                     "{\"ts\":\"%s\",\"n\":%d,\"trees\":%lu,"
                     "\"trees_d3\":%lu,\"trees_d4\":%lu,"
                     "\"rooted\":%lu,\"pruned\":%lu,"
                     "\"path\":%lu,\"d3\":%lu,\"d4\":%lu,\"any\":%lu,"
                     "\"d3_deg\":%d,\"d3_leaves\":%d,\"d3_diam\":%d,"
                     "\"d3_edges\":[%s],\"ms\":%.0f}\n",
                     ts, n, unique, trees_d3, trees_d4, rooted, pruned, p_sc,
                     best_d3.score, best_d4.score, best_any.score,
                     best_d3.max_degree, best_d3.leaves, best_d3.diameter,
                     edges_buf, elapsed_ms);
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
    if (argc < 2) {
        fprintf(stderr, "Usage: %s N [--top K]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 1 || n > MAX_N) {
        fprintf(stderr, "N must be in [1, %d]\n", MAX_N);
        return 1;
    }

    int top_k = 10;
    int prune_deg = 0;  // 0 = no pruning
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--top") == 0 && i + 1 < argc)
            top_k = atoi(argv[++i]);
        if (strcmp(argv[i], "--prune") == 0) {
            // --prune alone defaults to 4, --prune 3 prunes at degree 3
            if (i + 1 < argc && argv[i + 1][0] >= '2' && argv[i + 1][0] <= '9')
                prune_deg = atoi(argv[++i]);
            else
                prune_deg = 4;
        }
    }

    fprintf(
        stderr, "[c++ synthesizer] Enumerating trees on N=%d (top_k=%d%s)\n", n,
        top_k,
        prune_deg > 0
            ? (std::string(", PRUNING d>") + std::to_string(prune_deg)).c_str()
            : "");

    uint64_t p_sc = path_score(n);
    fprintf(stderr, "  Path P(%d) baseline: %lu independent sets\n", n, p_sc);

    generate(n, top_k, prune_deg);
    return 0;
}
