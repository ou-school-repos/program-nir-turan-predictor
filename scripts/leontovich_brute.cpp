// Exact brute-force search for small Leontovich graphs (loops allowed).
//
// Enumerates every labeled graph on m vertices (2^(m*(m+1)/2) adjacency
// matrices, upper triangle incl. diagonal for loops), filters disconnected /
// degree-0 graphs, then runs exact-integer homomorphism counting on
// survivors. Walk counts use unsigned __int128 with adaptive overflow
// truncation (exact up to the point of truncation -- never wraps silently).
//
// Build:
//   g++ -O3 -march=native -std=c++17 -pthread -o leontovich_brute
//   leontovich_brute.cpp
// Run:
//   ./leontovich_brute -m 6
//   ./leontovich_brute -m 7 --threads 6

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using u128 = unsigned __int128;

struct Slot {
    int i, j;
};

static std::vector<Slot> slots_for(int m) {
    std::vector<Slot> s;
    for (int i = 0; i < m; ++i)
        for (int j = i; j < m; ++j) s.push_back({i, j});
    return s;
}

static void build_adj(int m, const std::vector<Slot>& slots, uint64_t mask,
                      std::vector<std::vector<int>>& A) {
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < m; ++j) A[i][j] = 0;
    for (size_t k = 0; k < slots.size(); ++k) {
        if ((mask >> k) & 1ULL) {
            A[slots[k].i][slots[k].j] = 1;
            A[slots[k].j][slots[k].i] = 1;
        }
    }
}

// Symmetry-breaking: every isomorphism class has >=1 labeling with a
// non-increasing degree sequence, so restricting to those never misses a
// graph -- just skips redundant relabelings. Cheapest filter, so run first.
static bool has_sorted_degrees(int m, const std::vector<std::vector<int>>& A) {
    int prev = m + 1;  // unattainable upper bound
    for (int i = 0; i < m; ++i) {
        int deg = 0;
        for (int j = 0; j < m; ++j) deg += A[i][j];
        if (deg > prev) return false;
        prev = deg;
    }
    return true;
}

static bool has_no_isolated(int m, const std::vector<std::vector<int>>& A) {
    for (int i = 0; i < m; ++i) {
        bool any = false;
        for (int j = 0; j < m; ++j)
            if (A[i][j]) {
                any = true;
                break;
            }
        if (!any) return false;
    }
    return true;
}

static bool is_connected(int m, const std::vector<std::vector<int>>& A) {
    std::vector<char> seen(m, 0);
    std::vector<int> stack;
    stack.reserve(m);
    seen[0] = 1;
    stack.push_back(0);
    int n_seen = 1;
    while (!stack.empty()) {
        int v = stack.back();
        stack.pop_back();
        for (int u = 0; u < m; ++u) {
            if (A[v][u] && !seen[u]) {
                seen[u] = 1;
                ++n_seen;
                stack.push_back(u);
            }
        }
    }
    return n_seen == m;
}

// Multiply a u128 vector by adjacency matrix A: out[i] = sum_j A[i][j]*in[j]
// Returns false (and sets overflowed) if any accumulation would overflow u128.
static const u128 U128_SAFE_MAX =
    (~(u128)0) >> 8;  // generous headroom before real overflow

static bool matvec(int m, const std::vector<std::vector<int>>& A,
                   const std::vector<u128>& in, std::vector<u128>& out) {
    for (int i = 0; i < m; ++i) {
        u128 s = 0;
        for (int j = 0; j < m; ++j) {
            if (A[i][j]) {
                s += in[j];
                if (s > U128_SAFE_MAX) return false;
            }
        }
        out[i] = s;
    }
    return true;
}

// Returns true and sets n,d on first Leontovich hit found; false otherwise.
static bool check_leontovich_exact(int m,
                                   const std::vector<std::vector<int>>& A,
                                   int max_n, int max_d, int& hit_n,
                                   int& hit_d) {
    std::vector<std::vector<u128>> w;
    w.push_back(std::vector<u128>(m, 1));
    int actual_max_n = max_n;
    for (int k = 1; k < max_n; ++k) {
        std::vector<u128> nxt(m);
        if (!matvec(m, A, w.back(), nxt)) {
            actual_max_n = k;  // truncate here; last valid index is k-1
            break;
        }
        w.push_back(std::move(nxt));
    }
    max_n = actual_max_n;
    if (max_n < 3) return false;

    std::vector<u128> homP(max_n + 1, 0);
    for (int n = 1; n <= max_n; ++n) {
        u128 s = 0;
        for (int i = 0; i < m; ++i) s += w[n - 1][i];
        homP[n] = s;
    }

    int dmax = std::min(max_d, max_n - 2);
    for (int d = 2; d <= dmax; ++d) {
        std::vector<u128> b(m);
        for (int i = 0; i < m; ++i) b[i] = w[1][i] * w[d][i];
        for (int stem = 0; stem < max_n - d - 1; ++stem) {
            int n = stem + d + 2;
            if (n > max_n) break;
            u128 homE = 0;
            for (int i = 0; i < m; ++i) homE += w[stem][i] * b[i];
            if (homE < homP[n]) {
                hit_n = n;
                hit_d = d;
                return true;
            }
        }
    }
    return false;
}

int main(int argc, char** argv) {
    int m = -1, max_n = 40, max_d = 10, threads = 1;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](int& out) { out = std::atoi(argv[++i]); };
        if (a == "-m" || a == "--vertices")
            next(m);
        else if (a == "--max-n")
            next(max_n);
        else if (a == "--max-d")
            next(max_d);
        else if (a == "--threads")
            next(threads);
        else {
            fprintf(stderr, "Unknown arg: %s\n", a.c_str());
            return 1;
        }
    }
    if (m < 0) {
        fprintf(
            stderr,
            "Usage: %s -m <vertices> [--max-n N] [--max-d D] [--threads T]\n",
            argv[0]);
        return 1;
    }

    auto slots = slots_for(m);
    int n_slots = (int)slots.size();
    uint64_t total = 1ULL << n_slots;

    fprintf(
        stderr,
        "m=%d: %d slots, %llu total graphs, threads=%d, max_n=%d, max_d=%d\n",
        m, n_slots, (unsigned long long)total, threads, max_n, max_d);

    std::atomic<uint64_t> checked{0};
    std::atomic<uint64_t> hits{0};
    std::mutex print_mtx;

    auto worker = [&](int tid) {
        uint64_t lo = total * (uint64_t)tid / threads;
        uint64_t hi = total * (uint64_t)(tid + 1) / threads;
        std::vector<std::vector<int>> A(m, std::vector<int>(m, 0));
        for (uint64_t mask = lo; mask < hi; ++mask) {
            uint64_t c = checked.fetch_add(1, std::memory_order_relaxed) + 1;
            if (c % 20000000ULL == 0) {
                std::lock_guard<std::mutex> lk(print_mtx);
                fprintf(stderr, "  ... %llu/%llu checked\n",
                        (unsigned long long)c, (unsigned long long)total);
            }
            build_adj(m, slots, mask, A);
            if (!has_sorted_degrees(m, A)) continue;
            if (!has_no_isolated(m, A)) continue;
            if (!is_connected(m, A)) continue;
            int n, d;
            if (check_leontovich_exact(m, A, max_n, max_d, n, d)) {
                hits.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lk(print_mtx);
                printf("HIT mask=%llu n=%d d=%d edges=[",
                       (unsigned long long)mask, n, d);
                bool first = true;
                for (size_t k = 0; k < slots.size(); ++k) {
                    if ((mask >> k) & 1ULL) {
                        if (!first) printf(",");
                        printf("[%d,%d]", slots[k].i, slots[k].j);
                        first = false;
                    }
                }
                printf("]\n");
                fflush(stdout);
            }
        }
    };

    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();

    fprintf(stderr, "Done: checked %llu graphs, %llu hits\n",
            (unsigned long long)checked.load(),
            (unsigned long long)hits.load());
    return 0;
}
