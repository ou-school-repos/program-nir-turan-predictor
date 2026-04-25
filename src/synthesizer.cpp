// synthesizer.cpp — High-Performance Topology Discovery Engine
//
// Generates all non-isomorphic free trees on N vertices via
// Beyer-Hedetniemi level sequence enumeration with canonical
// 128-bit hashing and open-addressing deduplication.
//
// Build: g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static constexpr int MAX_N = 64;
static constexpr int MAX_EDGES = MAX_N * 2;  // tree has N-1 edges, x2 directed

// =====================================================================
// 128-bit Hash
// =====================================================================
struct Hash128 {
    uint64_t h0, h1;
    bool operator==(const Hash128& o) const { return h0 == o.h0 && h1 == o.h1; }
};

static const Hash128 EMPTY_HASH = {0, 0};
static const Hash128 TOMBSTONE = {~0ULL, ~0ULL};

inline uint64_t splitmix64(uint64_t z) {
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

inline uint64_t mix2(uint64_t z) {
    z += 0x5555555555555555ULL;
    z = (z ^ (z >> 33)) * 0x85ebca6bULL;
    z = (z ^ (z >> 29)) * 0xc2b2ae35ULL;
    return z ^ (z >> 32);
}

// =====================================================================
// Open-Addressing Flat Hash Set (128-bit keys, linear probing)
// =====================================================================
struct FlatHashSet {
    Hash128* keys;
    uint64_t cap;
    uint64_t mask;
    uint64_t count;

    FlatHashSet() : keys(nullptr), cap(0), mask(0), count(0) {}

    void init(uint64_t expected) {
        // Next power of 2, with ~50% load factor
        cap = 1;
        while (cap < expected * 2) cap <<= 1;
        mask = cap - 1;
        keys = new Hash128[cap];
        memset(keys, 0, cap * sizeof(Hash128));
        count = 0;
    }

    ~FlatHashSet() { delete[] keys; }

    // Returns true if newly inserted, false if already present.
    bool insert(Hash128 h) {
        uint64_t idx = h.h0 & mask;
        while (true) {
            Hash128& slot = keys[idx];
            if (slot == EMPTY_HASH) {
                slot = h;
                count++;
                return true;
            }
            if (slot == h) return false;
            idx = (idx + 1) & mask;
        }
    }

    uint64_t size() const { return count; }
};

// =====================================================================
// Flat Adjacency List (zero allocation)
// =====================================================================
static int head[MAX_N];
static int to_[MAX_EDGES];
static int nxt[MAX_EDGES];
static int edge_count;

static void adj_clear(int n) {
    memset(head, -1, n * sizeof(int));
    edge_count = 0;
}

static void adj_add_edge(int u, int v) {
    to_[edge_count] = v;
    nxt[edge_count] = head[u];
    head[u] = edge_count++;
    to_[edge_count] = u;
    nxt[edge_count] = head[v];
    head[v] = edge_count++;
}

// =====================================================================
// Parent array from level sequence (O(N), stack-based)
// =====================================================================
static int parent_arr[MAX_N];
static int depth_stack[MAX_N];

static void level_seq_to_parent(const int L[], int n) {
    // depth_stack[d] = last node at depth d
    parent_arr[0] = -1;
    depth_stack[0] = 0;
    for (int i = 1; i < n; i++) {
        parent_arr[i] = depth_stack[L[i] - 1];
        depth_stack[L[i]] = i;
    }
}

// Build flat adj list from parent array
static void parent_to_adj(int n) {
    adj_clear(n);
    for (int i = 1; i < n; i++) {
        adj_add_edge(parent_arr[i], i);
    }
}

// =====================================================================
// Bottom-up DP for Independent Set count (iterative, O(N))
// =====================================================================
static uint64_t dp_excl[MAX_N];
static uint64_t dp_incl[MAX_N];

static uint64_t compute_is_count(int n) {
    // Process nodes in reverse order (children before parents in preorder)
    for (int i = 0; i < n; i++) {
        dp_excl[i] = 1;
        dp_incl[i] = 1;
    }
    for (int i = n - 1; i > 0; i--) {
        int p = parent_arr[i];
        dp_excl[p] *= (dp_excl[i] + dp_incl[i]);
        dp_incl[p] *= dp_excl[i];
    }
    return dp_excl[0] + dp_incl[0];
}

// Path score (Fibonacci-like)
static uint64_t path_score(int n) {
    if (n <= 0) return 1;
    if (n == 1) return 2;
    uint64_t a = 1, b = 2;
    for (int i = 2; i <= n; i++) {
        uint64_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

// =====================================================================
// Tree Invariants (computed from parent array, O(N))
// =====================================================================
struct TreeInvariants {
    int diameter;
    int leaves;
    int max_degree;
};

static int degree[MAX_N];
static int height[MAX_N];

static TreeInvariants compute_invariants(int n) {
    TreeInvariants inv = {};

    // Compute degree and leaf count
    memset(degree, 0, n * sizeof(int));
    for (int i = 1; i < n; i++) {
        degree[i]++;
        degree[parent_arr[i]]++;
    }
    for (int i = 0; i < n; i++) {
        if (degree[i] == 1 || (i == 0 && n == 1)) inv.leaves++;
        if (degree[i] > inv.max_degree) inv.max_degree = degree[i];
    }

    // Diameter via height DP (bottom-up)
    memset(height, 0, n * sizeof(int));
    inv.diameter = 0;
    for (int i = n - 1; i > 0; i--) {
        int p = parent_arr[i];
        int new_h = height[i] + 1;
        // Diameter candidate: longest path through p
        inv.diameter = std::max(inv.diameter, height[p] + new_h);
        height[p] = std::max(height[p], new_h);
    }

    return inv;
}

// =====================================================================
// Canonical 128-bit Hash (center-rooted DFS, sorted children)
// =====================================================================
static Hash128 canon_hash_dfs(int u, int par) {
    Hash128 children[MAX_N];  // stack-local: safe across recursion
    int nc = 0;
    for (int e = head[u]; e != -1; e = nxt[e]) {
        int v = to_[e];
        if (v == par) continue;
        children[nc++] = canon_hash_dfs(v, u);
    }
    // Sort children hashes for canonical form
    std::sort(children, children + nc, [](const Hash128& a, const Hash128& b) {
        return a.h0 != b.h0 ? a.h0 < b.h0 : a.h1 < b.h1;
    });

    // Combine: seed with degree, fold in children
    uint64_t h0 = splitmix64((uint64_t)nc + 0xCAFEBABE);
    uint64_t h1 = mix2((uint64_t)nc + 0xDEADBEEF);
    for (int i = 0; i < nc; i++) {
        h0 ^= splitmix64(children[i].h0 + (uint64_t)(i + 1));
        h1 ^= mix2(children[i].h1 + (uint64_t)(i + 1));
    }
    return {h0, h1};
}

// Find center(s) and compute canonical hash
static Hash128 tree_hash_128(int n) {
    // Leaf peeling to find center(s)
    int deg[MAX_N];
    bool removed[MAX_N] = {};
    int leaves_buf[MAX_N];
    int leaf_count = 0;
    int remaining = n;

    for (int i = 0; i < n; i++) {
        deg[i] = degree[i];  // reuse from compute_invariants
        if (deg[i] <= 1) leaves_buf[leaf_count++] = i;
    }

    int next_buf[MAX_N];
    while (remaining > 2) {
        int next_count = 0;
        for (int k = 0; k < leaf_count; k++) {
            int v = leaves_buf[k];
            removed[v] = true;
            remaining--;
            for (int e = head[v]; e != -1; e = nxt[e]) {
                int u = to_[e];
                if (!removed[u] && --deg[u] == 1) next_buf[next_count++] = u;
            }
        }
        leaf_count = next_count;
        memcpy(leaves_buf, next_buf, next_count * sizeof(int));
    }

    // Root at center, compute canonical hash
    if (leaf_count == 1) {
        return canon_hash_dfs(leaves_buf[0], -1);
    } else {
        // Bicentral: try both roots, take lexicographically smaller
        Hash128 c0 = canon_hash_dfs(leaves_buf[0], -1);
        Hash128 c1 = canon_hash_dfs(leaves_buf[1], -1);
        Hash128 combined;
        Hash128 lo =
            (c0.h0 < c1.h0 || (c0.h0 == c1.h0 && c0.h1 <= c1.h1)) ? c0 : c1;
        Hash128 hi = (lo == c0) ? c1 : c0;
        combined.h0 = lo.h0 ^ splitmix64(hi.h0);
        combined.h1 = lo.h1 ^ mix2(hi.h1);
        return combined;
    }
}

// =====================================================================
// Top-K Tracking
// =====================================================================
struct TopKEntry {
    uint64_t score;
    int diameter;
    int leaves;
    int max_degree;
    int level_seq[MAX_N];
    int n;

    bool operator>(const TopKEntry& o) const { return score > o.score; }
};

static std::vector<TopKEntry> top_k_vec;
static int top_k_limit;
static uint64_t top_k_min_score;

static void top_k_init(int k) {
    top_k_vec.clear();
    top_k_vec.reserve(k + 1);
    top_k_limit = k;
    top_k_min_score = 0;
}

static void top_k_add(uint64_t score, const TreeInvariants& inv, const int L[],
                      int n) {
    if ((int)top_k_vec.size() >= top_k_limit && score <= top_k_min_score)
        return;

    TopKEntry e;
    e.score = score;
    e.diameter = inv.diameter;
    e.leaves = inv.leaves;
    e.max_degree = inv.max_degree;
    e.n = n;
    memcpy(e.level_seq, L, n * sizeof(int));

    top_k_vec.push_back(e);
    std::sort(top_k_vec.begin(), top_k_vec.end(),
              [](const TopKEntry& a, const TopKEntry& b) {
                  return a.score > b.score;
              });
    if ((int)top_k_vec.size() > top_k_limit) top_k_vec.pop_back();
    top_k_min_score = top_k_vec.back().score;
}

// =====================================================================
// Main Generation Loop (Beyer-Hedetniemi + inline DP + hash)
// =====================================================================
static uint64_t generate(int n, int top_k) {
    // A000055 approximate counts for hash set sizing
    static const uint64_t a000055[] = {0,
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
    uint64_t expected = (n <= 30) ? a000055[n] : a000055[30];

    FlatHashSet seen;
    seen.init(expected);

    top_k_init(top_k);
    auto t_start = std::chrono::high_resolution_clock::now();

    // Initial level sequence: the path graph (0, 1, 2, ..., n-1)
    int L[MAX_N];
    for (int i = 0; i < n; i++) L[i] = i;

    uint64_t unique = 0;
    uint64_t last_reported = 0;

    while (true) {
        // 1. Parent array from level sequence (O(N), zero alloc)
        level_seq_to_parent(L, n);

        // 2. Bottom-up DP for IS count (O(N), zero alloc)
        uint64_t score = compute_is_count(n);

        // 3. Compute invariants (O(N))
        TreeInvariants inv = compute_invariants(n);

        // 4. Build flat adj list and compute canonical hash
        parent_to_adj(n);
        Hash128 h = tree_hash_128(n);

        // 5. Dedup via flat hash set
        if (seen.insert(h)) {
            unique++;
            top_k_add(score, inv, L, n);
        }

        // Progress reporting
        if (unique >= last_reported + 100000) {
            last_reported = unique;
            auto now = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(now - t_start)
                            .count();
            double rate = unique / (ms / 1000.0);
            uint64_t target = (n <= 30) ? a000055[n] : 0;
            if (target > 0) {
                fprintf(stderr, "  [c++] %luK / %luK trees | %.1fs | %.0fK/s\n",
                        unique / 1000, target / 1000, ms / 1000.0,
                        rate / 1000.0);
            } else {
                fprintf(stderr, "  [c++] %luK trees | %.1fs | %.0fK/s\n",
                        unique / 1000, ms / 1000.0, rate / 1000.0);
            }
        }

        // Beyer-Hedetniemi successor
        int p = n - 1;
        while (p > 0 && L[p] <= 1) p--;
        if (p == 0) break;  // exhausted all trees

        int q = p - 1;
        while (L[q] != L[p] - 1) q--;

        int period = p - q;
        for (int i = p; i < n; i++) L[i] = L[i - period];
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    fprintf(stderr, "  Scanned %lu trees in %.1f ms (%.0f trees/sec)\n", unique,
            elapsed_ms, unique / (elapsed_ms / 1000.0));

    // JSON output
    uint64_t p_sc = path_score(n);
    printf("{\n");
    printf("  \"n\": %d,\n", n);
    printf("  \"trees_scanned\": %lu,\n", unique);
    printf("  \"path_score\": %lu,\n", p_sc);
    printf("  \"elapsed_ms\": %.1f,\n", elapsed_ms);
    printf("  \"trees_per_sec\": %.0f,\n", unique / (elapsed_ms / 1000.0));
    printf("  \"top_k\": [\n");

    for (int r = 0; r < (int)top_k_vec.size(); r++) {
        const auto& e = top_k_vec[r];

        // Reconstruct adjacency list from level sequence for JSON
        int par[MAX_N], ds[MAX_N];
        par[0] = -1;
        ds[0] = 0;
        for (int i = 1; i < e.n; i++) {
            par[i] = ds[e.level_seq[i] - 1];
            ds[e.level_seq[i]] = i;
        }

        printf("    {\n");
        printf("      \"rank\": %d,\n", r + 1);
        printf("      \"score\": %lu,\n", e.score);
        printf("      \"ratio\": %.2f,\n", (double)e.score / p_sc);
        printf("      \"diameter\": %d,\n", e.diameter);
        printf("      \"leaves\": %d,\n", e.leaves);
        printf("      \"max_degree\": %d,\n", e.max_degree);

        // Degree sequence
        int dg[MAX_N] = {};
        for (int i = 1; i < e.n; i++) {
            dg[i]++;
            dg[par[i]]++;
        }
        printf("      \"degree_sequence\": [");
        for (int i = 0; i < e.n; i++) {
            printf("%d", dg[i]);
            if (i + 1 < e.n) printf(", ");
        }
        printf("],\n");

        // Edge list (compact)
        printf("      \"edges\": [");
        bool first = true;
        for (int i = 1; i < e.n; i++) {
            if (!first) printf(", ");
            printf("[%d,%d]", par[i], i);
            first = false;
        }
        printf("]\n");
        printf("    }%s\n", (r + 1 < (int)top_k_vec.size()) ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");

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
    if (n < 3 || n > MAX_N) {
        fprintf(stderr, "N must be in [3, %d]\n", MAX_N);
        return 1;
    }

    int top_k = 10;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--top") == 0 && i + 1 < argc)
            top_k = atoi(argv[++i]);
    }

    fprintf(stderr, "[c++ synthesizer] Enumerating trees on N=%d (top_k=%d)\n",
            n, top_k);

    uint64_t p_sc = path_score(n);
    fprintf(stderr, "  Path P(%d) baseline: %lu independent sets\n", n, p_sc);

    generate(n, top_k);
    return 0;
}
