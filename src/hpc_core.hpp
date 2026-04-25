// hpc_core.hpp — Shared HPC primitives for topology discovery
//
// Zero-allocation graph primitives, 128-bit hashing, and
// cache-friendly data structures.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

static constexpr int MAX_N = 64;
static constexpr int MAX_EDGES = MAX_N * 2;

// =====================================================================
// 128-bit Hash
// =====================================================================
struct Hash128 {
    uint64_t h0, h1;
    bool is_empty() const { return h0 == 0 && h1 == 0; }
    bool operator==(const Hash128& o) const { return h0 == o.h0 && h1 == o.h1; }
};

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
        cap = 1;
        while (cap < expected * 2) cap <<= 1;
        mask = cap - 1;
        keys = (Hash128*)calloc(cap, sizeof(Hash128));
        count = 0;
    }

    ~FlatHashSet() { free(keys); }

    // Returns true if newly inserted, false if already present.
    bool insert(Hash128 h) {
        if (h.is_empty()) h.h0 = 1;  // Reserve {0,0} for empty sentinel
        uint64_t idx = h.h0 & mask;
        while (true) {
            Hash128& slot = keys[idx];
            if (slot.is_empty()) {
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
static int adj_head[MAX_N];
static int adj_to[MAX_EDGES];
static int adj_nxt[MAX_EDGES];
static int adj_ecnt;

static inline void adj_clear(int n) {
    memset(adj_head, -1, n * sizeof(int));
    adj_ecnt = 0;
}

static inline void adj_add_edge(int u, int v) {
    adj_to[adj_ecnt] = v;
    adj_nxt[adj_ecnt] = adj_head[u];
    adj_head[u] = adj_ecnt++;
    adj_to[adj_ecnt] = u;
    adj_nxt[adj_ecnt] = adj_head[v];
    adj_head[v] = adj_ecnt++;
}

// =====================================================================
// Parent array from level sequence (O(N), stack-based)
// =====================================================================
static int parent_arr[MAX_N];
static int depth_stack[MAX_N];

static inline void level_seq_to_parent(const int L[], int n) {
    parent_arr[0] = -1;
    depth_stack[0] = 0;
    for (int i = 1; i < n; i++) {
        parent_arr[i] = depth_stack[L[i] - 1];
        depth_stack[L[i]] = i;
    }
}

static inline void parent_to_adj(int n) {
    adj_clear(n);
    for (int i = 1; i < n; i++) adj_add_edge(parent_arr[i], i);
}

// =====================================================================
// Path IS count (Fibonacci recurrence)
// =====================================================================
static inline uint64_t path_score(int n) {
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
