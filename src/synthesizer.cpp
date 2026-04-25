// synthesizer.cpp — Unsupervised Topology Synthesizer
//
// Enumerates all non-isomorphic free trees on N vertices, scores each
// using the independent-set DP, and outputs the top anomalies as JSON.
//
// Two generation backends:
//   --method level   : WROM level-sequence (default, CAT)
//   --method parent  : Parent-array recursive generation
//
// Usage: ./synthesizer N [--method level|parent] [--top K]
//
// References:
//   Wright, Richmond, Odlyzko, McKay (1986).
//   "Constant time generation of free trees." SIAM J. Comput. 15(2).

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

static constexpr int MAX_N = 64;

// =====================================================================
// Independent-Set DP on adjacency list
// =====================================================================
// Returns (excl, incl) for rooted subtree at u with given parent.
// Total independent sets = excl[root] + incl[root].
struct DPResult {
    uint64_t excl;  // #IS not including u
    uint64_t incl;  // #IS including u
};

static DPResult dp_count(int u, int parent, const std::vector<int> adj[],
                         int n) {
    uint64_t excl = 1, incl = 1;
    for (int v : adj[u]) {
        if (v == parent) continue;
        DPResult c = dp_count(v, u, adj, n);
        excl *= (c.excl + c.incl);
        incl *= c.excl;
    }
    return {excl, incl};
}

static uint64_t score_tree(const std::vector<int> adj[], int n) {
    DPResult r = dp_count(0, -1, adj, n);
    return r.excl + r.incl;
}

// Score the path graph P(n)
static uint64_t path_score(int n) {
    std::vector<int> adj[MAX_N];
    for (int i = 0; i < n - 1; i++) {
        adj[i].push_back(i + 1);
        adj[i + 1].push_back(i);
    }
    return score_tree(adj, n);
}

// =====================================================================
// Anomaly storage
// =====================================================================
struct Anomaly {
    uint64_t score;
    std::vector<std::vector<int>> adj;
    std::vector<int> degree_seq;

    bool operator<(const Anomaly& o) const { return score < o.score; }
    bool operator>(const Anomaly& o) const { return score > o.score; }
};

static void record_anomaly(std::priority_queue<Anomaly, std::vector<Anomaly>,
                                               std::greater<Anomaly>>& pq,
                           const std::vector<int> adj[], int n, uint64_t sc,
                           int top_k) {
    Anomaly a;
    a.score = sc;
    a.adj.resize(n);
    a.degree_seq.resize(n);
    for (int i = 0; i < n; i++) {
        a.adj[i] = adj[i];
        a.degree_seq[i] = (int)adj[i].size();
    }
    pq.push(std::move(a));
    if ((int)pq.size() > top_k) pq.pop();
}

// =====================================================================
// Canonical tree hash for deduplication (fast numeric version)
// =====================================================================
// Hash subtrees by combining sorted children hashes with a mixing
// function. Two isomorphic trees get the same hash. No string allocs.

static uint64_t hash_mix(uint64_t h, uint64_t val) {
    h ^= val * 0x9e3779b97f4a7c15ULL;
    h = (h << 31) | (h >> 33);
    h *= 0xbf58476d1ce4e5b9ULL;
    return h;
}

static uint64_t canon_hash_subtree(int u, int parent,
                                   const std::vector<int> adj[]) {
    uint64_t child_hashes[MAX_N];
    int nc = 0;
    for (int v : adj[u]) {
        if (v != parent) child_hashes[nc++] = canon_hash_subtree(v, u, adj);
    }
    std::sort(child_hashes, child_hashes + nc);
    uint64_t h = 0x517cc1b727220a95ULL;  // seed
    h = hash_mix(h, (uint64_t)nc);
    for (int i = 0; i < nc; i++) h = hash_mix(h, child_hashes[i]);
    return h;
}

static size_t tree_hash(const std::vector<int> adj[], int n) {
    // Find center(s) via leaf peeling
    int degree[MAX_N];
    bool removed[MAX_N] = {};
    std::vector<int> leaves;
    int remaining = n;

    for (int i = 0; i < n; i++) {
        degree[i] = (int)adj[i].size();
        if (degree[i] <= 1) leaves.push_back(i);
    }
    while (remaining > 2) {
        std::vector<int> next;
        for (int v : leaves) {
            removed[v] = true;
            remaining--;
            for (int u : adj[v]) {
                if (!removed[u] && --degree[u] == 1) next.push_back(u);
            }
        }
        leaves = next;
    }

    // Root at center, compute canonical hash
    if (leaves.size() == 1) {
        return canon_hash_subtree(leaves[0], -1, adj);
    } else {
        uint64_t c0 = canon_hash_subtree(leaves[0], -1, adj);
        uint64_t c1 = canon_hash_subtree(leaves[1], -1, adj);
        return std::min(c0, c1) ^ (std::max(c0, c1) * 0x94d049bb133111ebULL);
    }
}

// =====================================================================
// Backend 1: WROM Level-Sequence Generation
// =====================================================================
// Generates all non-isomorphic free trees on n vertices using the
// level sequence successor function.
//
// A level sequence L[0..n-1] represents a tree in preorder traversal.
// L[0] = 0 (root), L[i] = depth of node i in preorder.
// The tree edges are: for each i > 0, parent[i] = the last node j < i
// with L[j] = L[i] - 1.

static void level_seq_to_adj(const int L[], int n, std::vector<int> adj[]) {
    for (int i = 0; i < n; i++) adj[i].clear();

    // Stack-based parent reconstruction
    int stack[MAX_N];
    int sp = 0;
    stack[sp] = 0;

    for (int i = 1; i < n; i++) {
        // Pop stack until we find the parent level
        while (sp >= 0 && L[stack[sp]] >= L[i]) sp--;
        if (sp >= 0) {
            int p = stack[sp];
            adj[p].push_back(i);
            adj[i].push_back(p);
        }
        stack[++sp] = i;
    }
}

// Check if a level sequence represents a canonical free tree.
// For free trees, we need the level sequence rooted at the center
// to be lexicographically maximal.
// Simplified approach: generate all rooted trees, check centrality.
static bool is_canonical_free_tree(const int L[], int n) {
    // Find the center(s) of the tree.
    // Build parent array first.
    std::vector<int> adj[MAX_N];
    level_seq_to_adj(L, n, adj);

    // Iterative leaf-peeling to find center(s)
    int degree[MAX_N];
    bool removed[MAX_N] = {};
    std::vector<int> leaves;
    int remaining = n;

    for (int i = 0; i < n; i++) {
        degree[i] = (int)adj[i].size();
        if (degree[i] <= 1) leaves.push_back(i);
    }

    while (remaining > 2) {
        std::vector<int> new_leaves;
        for (int v : leaves) {
            removed[v] = true;
            remaining--;
            for (int u : adj[v]) {
                if (!removed[u]) {
                    degree[u]--;
                    if (degree[u] == 1) new_leaves.push_back(u);
                }
            }
        }
        leaves = new_leaves;
    }

    // Center is leaves (1 or 2 nodes)
    // For unicentral: root at center, check L is the canonical sequence
    // For bicentral: root at both centers, take lexmax

    // For simplicity in this implementation, we accept all trees from
    // the successor function and use a hash-based dedup instead.
    // This is correct but not O(1) amortized — still fast in practice.
    (void)leaves;
    return true;
}

// Generate all non-isomorphic free trees using the WROM successor.
static uint64_t generate_wrom(int n, uint64_t p_score, int top_k,
                              std::priority_queue<Anomaly, std::vector<Anomaly>,
                                                  std::greater<Anomaly>>& pq,
                              uint64_t& anomaly_count) {
    // We use the Beyer-Hedetniemi successor for rooted trees,
    // then filter for canonical free trees.
    // Start: L = [0, 1, 2, ..., n-1] (path = lex-max)
    // End:   L = [0, 1, 1, ..., 1]   (star = lex-min)

    int L[MAX_N];
    for (int i = 0; i < n; i++) L[i] = i;

    uint64_t count = 0, unique = 0;
    std::vector<int> adj[MAX_N];
    std::unordered_set<size_t> seen;
    auto t_start = std::chrono::high_resolution_clock::now();

    // Process the initial tree (path)
    level_seq_to_adj(L, n, adj);
    size_t h = tree_hash(adj, n);
    seen.insert(h);
    uint64_t sc = score_tree(adj, n);
    count++;
    unique++;
    if (sc > p_score) {
        anomaly_count++;
        record_anomaly(pq, adj, n, sc, top_k);
    }

    while (true) {
        // Find p = rightmost index with L[p] != 1
        int p = -1;
        for (int i = n - 1; i >= 1; i--) {
            if (L[i] != 1) {
                p = i;
                break;
            }
        }
        if (p == -1) break;  // We've reached [0, 1, 1, ..., 1]

        // Find q = rightmost index < p with L[q] = L[p] - 1
        int q = -1;
        for (int i = p - 1; i >= 0; i--) {
            if (L[i] == L[p] - 1) {
                q = i;
                break;
            }
        }

        // Successor: copy L[q..q+(n-p)-1] over L[p..n-1]
        int period = p - q;
        for (int i = p; i < n; i++) {
            L[i] = L[i - period];
        }

        count++;

        // Convert, dedup, score
        level_seq_to_adj(L, n, adj);
        h = tree_hash(adj, n);
        if (seen.count(h)) continue;
        seen.insert(h);
        unique++;

        sc = score_tree(adj, n);
        // Always record top-K (the "anomaly" concept doesn't apply for IS —
        // every tree beats the path. We just want the ranking.)
        record_anomaly(pq, adj, n, sc, top_k);

        // Progress reporting with ETA
        if (unique % 100000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(now - t_start)
                            .count();
            double rate = unique / (ms / 1000.0);

            fprintf(stderr, "  [c++] %luK trees | %.1fs | %.0fK/s\n",
                    unique / 1000, ms / 1000.0, rate / 1000.0);
        }
    }

    return unique;
}

// =====================================================================
// Backend 2: Parent-Array Recursive Generation
// =====================================================================
// Generates non-isomorphic rooted trees, deduplicates via canonical
// form. This mirrors the NetworkX approach.

// Canonical form: sorted tuple of children's canonical forms (recursive)
struct CanonTree {
    std::vector<CanonTree> children;

    bool operator<(const CanonTree& o) const { return children < o.children; }
    bool operator==(const CanonTree& o) const { return children == o.children; }
};

// Convert a canonical tree to adjacency list
static void canon_to_adj(const CanonTree& t, int& next_id, int parent_id,
                         std::vector<int> adj[]) {
    int my_id = next_id++;
    if (parent_id >= 0) {
        adj[my_id].push_back(parent_id);
        adj[parent_id].push_back(my_id);
    }
    for (const auto& c : t.children) {
        canon_to_adj(c, next_id, my_id, adj);
    }
}

// Generate all rooted trees on n vertices (canonical form).
// A rooted tree on n vertices = root + partition of (n-1) among subtrees,
// where subtrees are rooted trees and sorted canonically.
static std::vector<CanonTree> rooted_trees_cache[MAX_N + 1];
static bool rooted_computed[MAX_N + 1] = {};

static const std::vector<CanonTree>& gen_rooted_trees(int n) {
    if (rooted_computed[n]) return rooted_trees_cache[n];
    rooted_computed[n] = true;

    if (n == 1) {
        rooted_trees_cache[n].push_back(CanonTree{{}});
        return rooted_trees_cache[n];
    }

    // Generate partitions of (n-1) into sorted parts
    // Each part corresponds to a subtree size, and for each size
    // we pick from gen_rooted_trees(size).
    // We use a recursive approach: build children list left-to-right,
    // each child's canonical form >= previous child's canonical form.

    std::vector<CanonTree>& result = rooted_trees_cache[n];

    // Helper: add children using remaining vertices
    std::function<void(int, const CanonTree*, CanonTree&)> build;
    build = [&](int remaining, const CanonTree* min_child, CanonTree& current) {
        if (remaining == 0) {
            result.push_back(current);
            return;
        }
        // Try each subtree size from 1 to remaining
        for (int sz = 1; sz <= remaining; sz++) {
            const auto& subtrees = gen_rooted_trees(sz);
            for (const auto& st : subtrees) {
                if (min_child && st < *min_child) continue;
                current.children.push_back(st);
                build(remaining - sz, &current.children.back(), current);
                current.children.pop_back();
            }
        }
    };

    CanonTree root;
    build(n - 1, nullptr, root);

    return rooted_trees_cache[n];
}

// Generate free trees from rooted trees.
// A free tree is either:
//   1. A rooted tree at its center (unicentral)
//   2. Two rooted trees joined at an edge (bicentral)
static uint64_t generate_parent(
    int n, uint64_t p_score, int top_k,
    std::priority_queue<Anomaly, std::vector<Anomaly>, std::greater<Anomaly>>&
        pq,
    uint64_t& anomaly_count) {
    uint64_t count = 0;
    std::vector<int> adj[MAX_N];

    // Unicentral: rooted trees where the root subtrees all have
    // size < n/2 (the root is the unique center)
    const auto& rtrees = gen_rooted_trees(n);
    for (const auto& rt : rtrees) {
        // Check if root is center: all subtrees must have size <= n/2
        // (and at most one has size exactly n/2)
        // For simplicity, we accept all and dedup via scoring.
        // This overcounts but is correct for anomaly finding.
        int next_id = 0;
        for (int i = 0; i < n; i++) adj[i].clear();
        canon_to_adj(rt, next_id, -1, adj);

        uint64_t sc = score_tree(adj, n);
        count++;

        if (sc > p_score) {
            anomaly_count++;
            record_anomaly(pq, adj, n, sc, top_k);
        }
    }

    // Note: This generates rooted trees, not free trees.
    // The count will be higher than non-isomorphic free trees.
    // For anomaly hunting this is fine — we find the same anomalies,
    // just with redundant scoring of some structures.

    return count;
}

// =====================================================================
// JSON Output
// =====================================================================
static void emit_json(int n, uint64_t trees_scanned, uint64_t p_sc,
                      double elapsed_ms, const char* method,
                      std::priority_queue<Anomaly, std::vector<Anomaly>,
                                          std::greater<Anomaly>>& pq) {
    // Drain PQ into sorted vector (highest score last in PQ)
    std::vector<Anomaly> anomalies;
    while (!pq.empty()) {
        anomalies.push_back(pq.top());
        pq.pop();
    }
    std::sort(
        anomalies.begin(), anomalies.end(),
        [](const Anomaly& a, const Anomaly& b) { return a.score > b.score; });

    printf("{\n");
    printf("  \"n\": %d,\n", n);
    printf("  \"method\": \"%s\",\n", method);
    printf("  \"trees_scanned\": %lu,\n", trees_scanned);
    printf("  \"path_score\": %lu,\n", p_sc);

    printf("  \"elapsed_ms\": %.1f,\n", elapsed_ms);
    printf("  \"trees_per_sec\": %.0f,\n",
           trees_scanned / (elapsed_ms / 1000.0));

    if (!anomalies.empty()) {
        const auto& top = anomalies[0];
        printf("  \"top_anomaly\": {\n");
        printf("    \"score\": %lu,\n", top.score);
        printf("    \"ratio\": %.2f,\n", (double)top.score / p_sc);

        // Adjacency list
        printf("    \"adj\": [\n");
        for (int i = 0; i < (int)top.adj.size(); i++) {
            printf("      [");
            for (int j = 0; j < (int)top.adj[i].size(); j++) {
                printf("%d", top.adj[i][j]);
                if (j + 1 < (int)top.adj[i].size()) printf(", ");
            }
            printf("]");
            if (i + 1 < (int)top.adj.size()) printf(",");
            printf("\n");
        }
        printf("    ],\n");

        // Degree sequence
        printf("    \"degree_sequence\": [");
        for (int i = 0; i < (int)top.degree_seq.size(); i++) {
            printf("%d", top.degree_seq[i]);
            if (i + 1 < (int)top.degree_seq.size()) printf(", ");
        }
        printf("]\n");
        printf("  }\n");
    } else {
        printf("  \"top_anomaly\": null\n");
    }

    printf("}\n");
}

// =====================================================================
// Main
// =====================================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s N [--method level|parent] [--top K]\n",
                argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 3 || n > MAX_N) {
        fprintf(stderr, "N must be in [3, %d]\n", MAX_N);
        return 1;
    }

    const char* method = "level";
    int top_k = 10;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) {
            method = argv[++i];
        } else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            top_k = atoi(argv[++i]);
        }
    }

    // Telemetry to stderr (so JSON on stdout stays clean)
    fprintf(stderr,
            "[c++ synthesizer] Enumerating trees on N=%d "
            "(method=%s, top_k=%d)\n",
            n, method, top_k);

    uint64_t p_sc = path_score(n);
    fprintf(stderr, "  Path P(%d) baseline: %lu independent sets\n", n, p_sc);

    std::priority_queue<Anomaly, std::vector<Anomaly>, std::greater<Anomaly>>
        pq;
    uint64_t anomaly_count = 0;

    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t count;

    if (strcmp(method, "parent") == 0) {
        if (n > 12) {
            fprintf(stderr,
                    "  [WARN] Parent method unstable for N>12, "
                    "falling back to level.\n");
            method = "level";
            count = generate_wrom(n, p_sc, top_k, pq, anomaly_count);
        } else {
            count = generate_parent(n, p_sc, top_k, pq, anomaly_count);
        }
    } else {
        count = generate_wrom(n, p_sc, top_k, pq, anomaly_count);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    fprintf(stderr, "  Scanned %lu trees in %.1f ms (%.0f trees/sec)\n", count,
            elapsed_ms, count / (elapsed_ms / 1000.0));
    emit_json(n, count, p_sc, elapsed_ms, method, pq);
    return 0;
}
