// Fast asymptotic filter for Leontovich graphs using E_n^{(d)} trees.
// Compile: g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
// Usage:   geng -c 8 -q | ./leontovich_fast
//
// Tests whether any near-path tree E_n^{(d)} (path with pendant at
// depth d from end) beats P_n, for n up to 200 and d up to 20.
// Uses O(h) matrix-vector multiplies with normalization to prevent
// overflow, enabling instant asymptotic screening.

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static void parse_graph6(const char* g6, int& h, double A[32][32]) {
    if (strncmp(g6, ">>graph6<<", 10) == 0) g6 += 10;
    h = g6[0] - 63;
    memset(A, 0, 32 * 32 * sizeof(double));
    int k = 1, bit_pos = 5;
    for (int col = 1; col < h; col++) {
        for (int row = 0; row < col; row++) {
            int val = g6[k] - 63;
            if ((val >> bit_pos) & 1) {
                A[row][col] = 1.0;
                A[col][row] = 1.0;
            }
            if (--bit_pos < 0) {
                k++;
                bit_pos = 5;
            }
        }
    }
}

static void matvec(int h, const double A[32][32], const double x[32],
                   double y[32]) {
    for (int i = 0; i < h; i++) {
        y[i] = 0;
        for (int j = 0; j < h; j++) y[i] += A[i][j] * x[j];
    }
}

int main() {
    string line;
    int leontovich_count = 0;
    int total_count = 0;
    auto t0 = chrono::steady_clock::now();

    while (cin >> line) {
        if (line.empty()) continue;
        int h;
        double A[32][32];
        parse_graph6(line.c_str(), h, A);
        total_count++;

        // Progress every 10K graphs
        if (total_count % 10000 == 0) {
            auto now = chrono::steady_clock::now();
            double secs = chrono::duration<double>(now - t0).count();
            int rate = (int)(total_count / secs);
            cerr << "  [filter] " << total_count << " graphs | "
                 << leontovich_count << " hits | " << rate << "/s | "
                 << (int)secs << "s elapsed\r" << flush;
        }

        // Precompute A^k * 1 for k = 0..31
        double Ak1[32][32];
        for (int i = 0; i < h; i++) Ak1[0][i] = 1.0;
        for (int kk = 1; kk < 32; kk++) matvec(h, A, Ak1[kk - 1], Ak1[kk]);

        bool is_leo = false;

        // Test E_n^{(d)}: path with pendant at depth d from end
        for (int d = 2; d <= 20; d++) {
            double vE[32], vP[32];
            for (int i = 0; i < h; i++) {
                vE[i] = Ak1[1][i] * Ak1[d][i];
                vP[i] = Ak1[d + 1][i];
            }

            // Push vectors forward to n=200
            for (int n = d + 2; n <= 200; n++) {
                double homP = 0, homE = 0;
                for (int i = 0; i < h; i++) {
                    homP += vP[i];
                    homE += vE[i];
                }

                if (homE < homP * (1.0 - 1e-11)) {
                    is_leo = true;
                    if (d > 2) {
                        cout << "ANOMALY " << line << " |V|=" << h << " n=" << n
                             << " d=" << d << " (d>2 required to beat path!)"
                             << " hom(E)=" << homE << " < hom(P)=" << homP
                             << endl;
                    } else {
                        cout << "LEONTOVICH " << line << " |V|=" << h
                             << " n=" << n << " d=" << d << " hom(E)=" << homE
                             << " < hom(P)=" << homP << endl;
                    }
                    break;
                }

                double next_vE[32], next_vP[32];
                matvec(h, A, vE, next_vE);
                matvec(h, A, vP, next_vP);

                // Normalize to prevent overflow
                double norm = homP > 1.0 ? homP : 1.0;
                for (int i = 0; i < h; i++) {
                    vE[i] = next_vE[i] / norm;
                    vP[i] = next_vP[i] / norm;
                }
            }
            if (is_leo) break;
        }
        if (is_leo) leontovich_count++;
    }

    auto t1 = chrono::steady_clock::now();
    double total_secs = chrono::duration<double>(t1 - t0).count();
    int rate = total_count > 0 ? (int)(total_count / total_secs) : 0;
    cerr << "\nLeontovich filter: " << leontovich_count << " / " << total_count
         << " graphs flagged (n<=200, d<=20)"
         << " | " << (int)total_secs << "s"
         << " | " << rate << " graphs/s" << endl;

    // JSONL summary to stderr for pipeline auditing
    cerr << "{\"event\":\"leontovich_filter_done\""
         << ",\"total\":" << total_count << ",\"hits\":" << leontovich_count
         << ",\"elapsed_s\":" << (int)total_secs << ",\"rate_per_s\":" << rate
         << "}" << endl;

    return 0;
}
