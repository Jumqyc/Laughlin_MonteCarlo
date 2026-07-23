#ifndef LAUGHLIN_HPP
#define LAUGHLIN_HPP
#include <cmath>
#include <random>
#include <cstdint>

// ── xoshiro256** ── fast, high‑quality PRNG
// https://prng.di.unimi.it/
class Xoshiro256ss {
    uint64_t s[4];
    static inline uint64_t rotl(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }
public:
    using result_type = uint64_t;
    Xoshiro256ss() {
        std::random_device rd;
        for (auto &v : s)
            v = (uint64_t(rd()) << 32) | rd();
    }
    uint64_t operator()() {
        uint64_t r = rotl(s[1] * 5, 7) * 9;
        uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return r;
    }
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return UINT64_MAX; }
};

// Direct float generation:
inline float rand_01(uint64_t x) { return (x >> 11) * 0x1.0p-53f; }
inline float rand_pm(uint64_t x) { return (x >> 11) * 0x1.0p-52f - 1.0f; }


class Laughlin {
public:
    // without quasiholes
    Laughlin(float *int_mat, int *particle_numbers, int ptype_num,
             float bdr, float res);

    // with quasiholes
    Laughlin(float *int_mat, int *particle_numbers, int ptype_num,
             float bdr, float res,
             float *hole_positions, float *hole_charges, int n_holes);

    ~Laughlin();
    void run(int steps);
    int *get_histogram(void) { return this->histogram; }

    int get_bins()          const { return bins; }
    int get_num_types()     const { return n_types; }
    int get_total_particles() const { return n_tot; }
    int get_num_holes()     const { return n_holes; }

private:
    const int n_tot;
    const int n_types;
    int *types;
    float *positions;
    float *interactions;
    float *pair_r2;
    float *half_strength;
    float *temp_r2;
    bool use_fast_pow;
    int  fast_pow_k;          // uniform k (≥0) or −1

    // quasiholes
    int    n_holes;
    float *hole_x, *hole_y;   // positions
    int   *hole_k;            // precomputed k = round(2 × charge)

    const float boundary, resolution;
    const int   bins;
    int  *histogram;

    Xoshiro256ss gen;

    void update(int i);
    void record(void);
};

#endif
