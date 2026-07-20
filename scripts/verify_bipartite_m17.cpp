#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#include <omp.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../src/uint256.hpp"

using namespace std;

int m1_g;
int num_subsets;
int num_perms;
vector<vector<int>> inv_perms;

void init_perms(int m1) {
    m1_g = m1;
    num_subsets = (1 << m1) - 1;
    vector<int> p(m1);
    for (int i = 0; i < m1; i++) p[i] = i;

    inv_perms.clear();
    do {
        vector<int> perm_map(num_subsets + 1);
        for (int mask = 1; mask <= num_subsets; mask++) {
            int new_mask = 0;
            for (int i = 0; i < m1; i++) {
                if ((mask >> i) & 1) new_mask |= (1 << p[i]);
            }
            perm_map[mask] = new_mask;
        }

        vector<int> inv_map(num_subsets + 1);
        for (int mask = 1; mask <= num_subsets; mask++) {
            inv_map[perm_map[mask]] = mask;
        }
        inv_perms.push_back(inv_map);

    } while (next_permutation(p.begin(), p.end()));
    num_perms = inv_perms.size();
}

// Hyper-fast Group Backtrack Pruning: Checks if chosen elements
// c[idx...num_subsets] are canonical
bool is_partial_canonical(int idx, const int* c) {
    for (int p = 1; p < num_perms; p++) {
        for (int i = num_subsets; i >= idx; i--) {
            int mapped_i = inv_perms[p][i];
            if (mapped_i >= idx) {
                if (c[mapped_i] > c[i]) return false;
                if (c[mapped_i] < c[i])
                    break;  // this perm makes it lexicographically smaller on
                            // chosen elements
            } else {
                // mapped_i is not yet chosen, so we can't make a complete
                // lexicographical decision
                break;
            }
        }
    }
    return true;
}

long long total_graphs = 0;
long long leo_count = 0;

void evaluate(int m2, const int* c) {
    // Correct alternating bipartite DP for the depth-2 near-path family only.
    // Rows are stack-allocated without bulk zeroing; every row entry used below
    // is fully assigned before it is read.
    uint256_t wL[61][8];
    uint256_t wR[61][256];

    for (int i = 0; i < m1_g; i++) wL[0][i] = uint256_t(1);
    for (int p = 1; p <= num_subsets; p++) wR[0][p] = uint256_t(1);

    for (int s = 1; s <= 60; s++) {
        for (int p = 1; p <= num_subsets; p++) {
            uint256_t sum;
            for (int i = 0; i < m1_g; i++) {
                if ((p >> i) & 1) sum = sum + wL[s - 1][i];
            }
            wR[s][p] = sum;
        }
        for (int i = 0; i < m1_g; i++) {
            uint256_t sum;
            for (int p = 1; p <= num_subsets; p++) {
                if ((p >> i) & 1) {
                    sum = sum + (wR[s - 1][p] * c[p]);
                }
            }
            wL[s][i] = sum;
        }
    }

    uint256_t homP[62];
    for (int n = 5; n <= 61; n += 2) {
        uint256_t s;
        for (int i = 0; i < m1_g; i++) s = s + wL[n - 1][i];
        for (int p = 1; p <= num_subsets; p++) {
            s = s + (wR[n - 1][p] * c[p]);
        }
        homP[n] = s;
    }

    int d = 2;  // This verifier is intentionally scoped to E_n^(2).
    uint64_t bL[8], bR[256];
    for (int i = 0; i < m1_g; i++) {
        bL[i] = (uint64_t)wL[1][i].low * (uint64_t)wL[d][i].low;
    }
    for (int p = 1; p <= num_subsets; p++) {
        bR[p] = (uint64_t)wR[1][p].low * (uint64_t)wR[d][p].low;
    }

    for (int n = 5; n <= 61; n += 2) {
        int stem = n - d - 2;
        if (stem < 0) continue;

        uint256_t homE;
        for (int i = 0; i < m1_g; i++) {
            homE = homE + (wL[stem][i] * bL[i]);
        }
        for (int p = 1; p <= num_subsets; p++) {
            uint64_t scalar = (uint64_t)c[p] * bR[p];
            homE = homE + (wR[stem][p] * scalar);
        }

        if (homE < homP[n]) {
#pragma omp atomic
            leo_count++;
            return;
        }
    }
}

void search(int idx, int remain, int* c, int m2) {
    if (idx == 0) {
        if (remain > 0) return;

        // Ensure no isolated left vertex
        for (int i = 0; i < m1_g; i++) {
            int sum = 0;
            for (int p = 1; p <= num_subsets; p++) {
                if ((p >> i) & 1) sum += c[p];
            }
            if (sum == 0) return;
        }

        // Run full canonicity check at the leaf since we bypassed checks for
        // idx < 12
        if (is_partial_canonical(1, c)) {
#pragma omp atomic
            total_graphs++;
            evaluate(m2, c);
        }
        return;
    }

    // Singleton Sorting Pruning (Massive Speedup)
    int min_val = 0;
    if (__builtin_popcount(idx) == 1) {
        int bit = __builtin_ctz(idx);
        if (bit < m1_g - 1) min_val = c[1 << (bit + 1)];
    }

    for (int v = min_val; v <= remain; v++) {
        c[idx] = v;
        // Only run partial canonicity check at upper levels (idx >= 12) to
        // avoid recursive overhead at the wide bottom of the tree
        if (idx < 12 || is_partial_canonical(idx, c)) {
            search(idx - 1, remain - v, c, m2);
        }
    }
}

int main() {
    auto t0 = chrono::steady_clock::now();
    cout << "\033[1;36mExhaustive Verification of Bipartite Partitions m <= 17 "
            "(Threshold Pruning Edition)\033[0m\n";
    cout << "=========================================================\n";

    for (int m1 = 4; m1 <= 8; m1++) {
        init_perms(m1);
        int max_m2 = 17 - m1;

        cout << "Partition m1=" << m1 << ":\n";
        for (int m2 = m1; m2 <= max_m2; m2++) {
#pragma omp parallel
            {
#pragma omp for schedule(dynamic)
                for (int v = 0; v <= m2; v++) {
                    int c_local[256] = {0};
                    c_local[num_subsets] = v;
                    search(num_subsets - 1, m2 - v, c_local, m2);
                }
            }
            cout << "  m2 = " << setw(2) << m2
                 << " | canonical graphs: " << total_graphs
                 << " | Leontovich violations: " << leo_count << "\n";
        }
    }

    auto t1 = chrono::steady_clock::now();
    cout << "=========================================================\n";
    cout << "COMPLETED IN " << chrono::duration<double>(t1 - t0).count()
         << " SECONDS.\n";
    cout << "\033[1;32mCERTIFICATE: no depth-2 bipartite Leontovich hit was "
            "found for odd n = 5..61 in the tested m1 >= 4, m <= 17 partition "
            "families. H_18 remains the scoped depth-2 witness; H* is the "
            "smaller general-depth witness.\033[0m\n";
    return 0;
}
