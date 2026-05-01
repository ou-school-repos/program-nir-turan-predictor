#pragma GCC optimize("O3,unroll-loops")
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

struct Edge {
    int u, v;
};

class CaterpillarGame {
    int S, K, N, NUM_EDGES, CUTS;
    vector<Edge> edges;
    unordered_map<uint64_t, int> memo;

    uint64_t spread_fire(uint64_t b, uint64_t e) {
        uint64_t nb = b;
        for (int i = 0; i < NUM_EDGES; i++) {
            if ((e >> i) & 1) {
                int u = edges[i].u, v = edges[i].v;
                if ((b >> u) & 1) nb |= (1ULL << v);
                if ((b >> v) & 1) nb |= (1ULL << u);
            }
        }
        return nb;
    }

    // Enumerate all subsets of frontier edges of size exactly `rem`,
    // cutting them from `e`, and return the minimum Nash value.
    // `start` avoids duplicate subsets by only considering edges >= start.
    int builder_choose(uint64_t b, uint64_t e, int rem, int start,
                       const vector<int>& frontier) {
        if (rem == 0) {
            // All cuts made; spread fire and recurse
            return builder_play(spread_fire(b, e), e);
        }
        int best = N;
        for (int j = start; j <= (int)frontier.size() - rem; j++) {
            int i = frontier[j];
            uint64_t ne = e & ~(1ULL << i);
            best = min(best, builder_choose(b, ne, rem - 1, j + 1, frontier));
        }
        return best;
    }

    int builder_play(uint64_t b, uint64_t e) {
        uint64_t state = b ^ (e * 0x9E3779B97F4A7C15ULL);
        if (memo.count(state)) return memo[state];

        uint64_t next_b = spread_fire(b, e);
        if (next_b == b) return __builtin_popcountll(b);

        // Collect frontier edges (one endpoint burned, one not)
        vector<int> frontier;
        for (int i = 0; i < NUM_EDGES; i++) {
            if ((e >> i) & 1) {
                int u = edges[i].u, v = edges[i].v;
                if (((b >> u) & 1) ^ ((b >> v) & 1)) {
                    frontier.push_back(i);
                }
            }
        }

        int min_val;
        if (frontier.empty()) {
            // No frontier edges to cut — fire spreads freely
            min_val = builder_play(next_b, e);
        } else {
            // Cut min(CUTS, |frontier|) edges
            int to_cut = min(CUTS, (int)frontier.size());
            min_val = builder_choose(b, e, to_cut, 0, frontier);
        }

        return memo[state] = min_val;
    }

   public:
    CaterpillarGame(int s, int k, int c = 1) : S(s), K(k), CUTS(c) {
        N = S + S * K;
        NUM_EDGES = (S - 1) + S * K;
        for (int i = 0; i < S - 1; i++) edges.push_back({i, i + 1});
        int idx = S;
        for (int i = 0; i < S; i++) {
            for (int j = 0; j < K; j++) edges.push_back({i, idx++});
        }
    }

    int solve() {
        int max_val = 0;
        for (int i = 0; i < S; i++) {
            memo.clear();
            max_val = max(max_val,
                          builder_play((1ULL << i), (1ULL << NUM_EDGES) - 1));
        }
        return max_val;
    }
};

static int C_MAX = 4;

int main(int argc, const char* argv[]) {
    if (argc > 1) C_MAX = atoi(argv[1]);
    // === Section 1: Original c=1 sweep (Model B) ===
    cout << "=== Model B Nash Sweep: c=1 (spread-after-cut) ===\n";
    cout << "--------+---------------------------------------------------------"
            "--------------+\n";
    cout << "  S \\ K |   0   |   1   |   2   |   3   |   4   |   5   |   6   "
            "|   7   |   8   |\n";
    cout << "--------+-------+-------+-------+-------+-------+-------+-------+-"
            "------+-------+\n";

    for (int s = 3; s <= 8; s++) {
        cout << "   " << s << "    |";
        for (int k = 0; k <= 8; k++) {
            if (s * (k + 1) > 63) {
                cout << "   --  |";
                continue;
            }
            CaterpillarGame game(s, k, 1);
            int nash = game.solve();
            cout << setw(5) << nash << "  |";
        }
        cout << "\n";
    }
    cout << "--------+-------+-------+-------+-------+-------+-------+-------+"
            "-------+-------+\n";
    cout << "Expected: 2K+1 for S<=4, 2K+2 for S>=5\n\n";

    // === Section 2: Multi-cut sweeps c=2,3 ===
    for (int c = 2; c <= C_MAX; c++) {
        cout << "=== Model B Nash Sweep: c=" << c
             << " (multi-cut, spread-after-cut) ===\n";
        cout << "--------+---------------------------------------+\n";
        cout << "  S \\ K |   0   |   1   |   2   |   3   |   4   |\n";
        cout << "--------+-------+-------+-------+-------+-------+\n";

        for (int s = 3; s <= 8; s++) {
            cout << "   " << s << "    |";
            for (int k = 0; k <= 4; k++) {
                if (s * (k + 1) > 40) {
                    cout << "   --  |";
                    continue;
                }
                CaterpillarGame game(s, k, c);
                int nash = game.solve();
                cout << setw(5) << nash << "  |";
            }
            cout << "\n";
        }
        cout << "--------+-------+-------+-------+-------+-------+\n";
        cout << "Predicted: max(1, K - " << c << " + 3) for all S >= 3\n\n";
    }

    // === Section 3: Verification matrix ===
    cout << "=== Verification: computed vs predicted ===\n";
    cout << "  c |  S |  K | computed | predicted | match\n";
    cout << "----+----+----+---------+-----------+------\n";

    int mismatches = 0;
    for (int c = 1; c <= C_MAX; c++) {
        for (int s = 3; s <= 7; s++) {
            for (int k = 0; k <= 4; k++) {
                if (s * (k + 1) > 40) continue;

                CaterpillarGame game(s, k, c);
                int computed = game.solve();

                // Predicted: c=1 → max(2, 2K+2 or 2K+1), c≥2 → max(1, K-c+3)
                int predicted = (c == 1) ? max(2, 2 * k + (s >= 5 ? 2 : 1))
                                         : max(1, k - c + 3);

                bool ok = (computed == predicted);
                if (!ok) mismatches++;

                cout << setw(3) << c << " |" << setw(3) << s << " |" << setw(3)
                     << k << " |" << setw(8) << computed << " |" << setw(10)
                     << predicted << " |" << (ok ? "  OK" : " FAIL") << "\n";
            }
        }
    }

    cout << "----+----+----+---------+-----------+------\n";
    cout << "Total mismatches: " << mismatches << "\n";

    return 0;
}
