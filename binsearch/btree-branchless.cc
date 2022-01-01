#include "binsearch.hh"
#include <x86intrin.h>
#include <sys/mman.h>

typedef __m256i reg;

#ifndef N
#define N (1<<20)
#endif

const int B = 16;
const int INF = std::numeric_limits<int>::max();

constexpr int height(int n) {
    int s = 0, // total size
        l = B, // size of next layer
        h = 0; // height so far
    while (s + l - B < n) {
        s += l;
        l *= (B + 1);
        h++;
    }
    return h;
}

const int nblocks = (N + B - 1) / B;
int *_a;
int (*btree)[B];

int blank[B] = {-1};

int go(int k, int i) { return k * (B + 1) + i + 1; }

void build(int k = 0) {
    static int t = 0;
    if (k < nblocks) {
        for (int i = 0; i < B; i++) {
            build(go(k, i));
            btree[k][i] = (t < N ? _a[t++] : INF);
        }
        build(go(k, B));
    }
}

void prepare(int *a, int _n) {
    //n = _n;
    //nblocks = (n + B - 1) / B;
    _a = a;
    btree = (int(*)[16]) std::aligned_alloc(2 * 1024 * 1024, 64 * nblocks);
    madvise(btree, 64 * nblocks, MADV_HUGEPAGE);
    build();
}

int cmp(reg x_vec, int* y_ptr) {
    reg y_vec = _mm256_load_si256((reg*) y_ptr);
    reg mask = _mm256_cmpgt_epi32(x_vec, y_vec);
    return _mm256_movemask_ps((__m256) mask);
}

int lower_bound(int x) {
    int k = 0, res = INF;
    reg x_vec = _mm256_set1_epi32(x);
    for (int h = 0; h < height(N); h++) {
        int mask = ~(
            cmp(x_vec, &btree[k][0]) +
            (cmp(x_vec, &btree[k][8]) << 8)
        );
        int i = __builtin_ffs(mask) - 1;
        if (i < B) [[likely]]
            res = btree[k][i];
        k = go(k, i);
    }
    int *loc = (k < nblocks ? btree[k] : blank);
    int mask = ~(
        cmp(x_vec, &loc[0]) +
        (cmp(x_vec, &loc[8]) << 8)
    );
    int i = __builtin_ffs(mask) - 1;
    if (i < B)
        res = btree[k][i];
    return res;
}
