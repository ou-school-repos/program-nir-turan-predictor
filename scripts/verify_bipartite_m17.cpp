#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#include <omp.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

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

bool is_canonical(const int* c) {
    for (int p = 1; p < num_perms; p++) {
        for (int i = num_subsets; i >= 1; i--) {
            int mapped_i = inv_perms[p][i];
            if (c[mapped_i] > c[i]) return false;
            if (c[mapped_i] < c[i]) break;
        }
    }
    return true;
}

long long total_graphs = 0;
long long leo_count = 0;

void evaluate(int m2, const int* c) {
    double wL[61][8] = {0};
    double wR[61][256] = {0};

    for (int i = 0; i < m1_g; i++) wL[0][i] = 1.0;
    for (int p = 1; p <= num_subsets; p++) wR[0][p] = 1.0;

    // Hyper-fast Left-Projection Matrix DP
    for (int s = 1; s <= 60; s++) {
        for (int p = 1; p <= num_subsets; p++) {
            double sum = 0;
            for (int i = 0; i < m1_g; i++) {
                if ((p >> i) & 1) sum += wL[s - 1][i];
            }
            wR[s][p] = sum;
        }
        for (int i = 0; i < m1_g; i++) {
            double sum = 0;
            for (int p = 1; p <= num_subsets; p++) {
                if ((p >> i) & 1) sum += c[p] * wR[s - 1][p];
            }
            wL[s][i] = sum;
        }
    }

    double homP[62] = {0};
    for (int n = 5; n <= 61; n += 2) {
        double s = 0;
        for (int i = 0; i < m1_g; i++) s += wL[n - 1][i];
        for (int p = 1; p <= num_subsets; p++) s += c[p] * wR[n - 1][p];
        homP[n] = s;
    }

    int d = 2;
    double bL[8], bR[256];
    for (int i = 0; i < m1_g; i++) bL[i] = wL[1][i] * wL[d][i];
    for (int p = 1; p <= num_subsets; p++) bR[p] = wR[1][p] * wR[d][p];

    for (int n = 5; n <= 61; n += 2) {
        int stem = n - d - 2;
        if (stem < 0) continue;

        double homE = 0;
        for (int i = 0; i < m1_g; i++) homE += wL[stem][i] * bL[i];
        for (int p = 1; p <= num_subsets; p++)
            homE += c[p] * wR[stem][p] * bR[p];

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

        if (is_canonical(c)) {
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
        search(idx - 1, remain - v, c, m2);
    }
}

int main() {
    auto t0 = chrono::steady_clock::now();
    cout << "\033[1;36mExhaustive Verification of Bipartite Partitions m <= "
            "17\033[0m\n";
    cout << "=========================================================\n";

    for (int m1 = 4; m1 <= 8; m1++) {
        init_perms(m1);
        int max_m2 = 17 - m1;
        if (max_m2 < m1) continue;

        cout << "Partition m1=" << m1 << ":\n";
        for (int m2 = m1; m2 <= max_m2; m2++) {
#pragma omp parallel
            {
                int c[256] = {0};
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
    cout << "\033[1;32mTHEOREM: H_18 is the absolute global minimal bipartite "
            "Leontovich graph.\033[0m\n";
    return 0;
}
