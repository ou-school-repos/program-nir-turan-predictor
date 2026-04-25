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
    int S, K, N, NUM_EDGES;
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

    int builder_play(uint64_t b, uint64_t e) {
        uint64_t state = b ^ (e * 0x9E3779B97F4A7C15ULL);
        if (memo.count(state)) return memo[state];

        uint64_t next_b = spread_fire(b, e);
        if (next_b == b) return __builtin_popcountll(b);

        int min_val = N;
        bool can_cut = false;

        for (int i = 0; i < NUM_EDGES; i++) {
            if ((e >> i) & 1) {
                int u = edges[i].u, v = edges[i].v;
                if (((b >> u) & 1) ^ ((b >> v) & 1)) {
                    can_cut = true;
                    uint64_t ne = e & ~(1ULL << i);
                    min_val =
                        min(min_val, builder_play(spread_fire(b, ne), ne));
                }
            }
        }
        if (!can_cut) min_val = builder_play(next_b, e);

        return memo[state] = min_val;
    }

   public:
    CaterpillarGame(int s, int k) : S(s), K(k) {
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

int main() {
    cout << "Caterpillar Nash Sweep (firefighter model: spread-after-cut)\n";
    cout << "--------+---------------------------------------------------------"
            "--------------+"
            "\n";
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
            CaterpillarGame game(s, k);
            int nash = game.solve();
            cout << setw(5) << nash << "  |";
        }
        cout << "\n";
    }
    cout << "--------+-------+-------+-------+-------+-------+-------+-------+"
            "-------+-------+\n";
    cout << "Compare: K+2 conjecture vs 2K+2 conjecture\n";
    return 0;
}
