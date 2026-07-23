#include "Laughlin.hpp"
#include <cmath>
#include <numeric>

inline float fast_half_int_pow(float x, int k)
{
    if (k == 0) return 1.0f;
    if (k == 1) return std::sqrt(x);
    if (k == 2) return x;
    if (k == 3) return x * std::sqrt(x);
    if (k == 4) return x * x;
    if (k == 5) return x * x * std::sqrt(x);

    int n = k >> 1;
    bool need_sqrt = k & 1;
    float result = 1.0f, base = x;
    while (n > 0) {
        if (n & 1) result *= base;
        base *= base;
        n >>= 1;
    }
    if (need_sqrt) result *= std::sqrt(x);
    return result;
}

// ── no‑hole constructor (delegates) ──────────────────────────────────
Laughlin::Laughlin(float *int_mat, int *particle_number_vect, int num_type,
                   float bdr, float res)
    : Laughlin(int_mat, particle_number_vect, num_type, bdr, res,
               nullptr, nullptr, 0) {}

// ── full constructor ─────────────────────────────────────────────────
Laughlin::Laughlin(float *int_mat, int *particle_number_vect, int num_type,
                   float bdr, float res,
                   float *hole_positions, float *hole_charges, int n_holes)
    : n_types(num_type),
      boundary(bdr),
      resolution(res),
      bins(static_cast<int>(2 * bdr / res) + 1),
      n_tot(std::accumulate(particle_number_vect,
                            particle_number_vect + num_type, 0)),
      n_holes(n_holes)
{
    // --- particle types ---
    this->types = new int[n_tot];
    int *type_ptr = types;
    for (int t = 0; t < num_type; ++t) {
        std::fill(type_ptr, type_ptr + particle_number_vect[t], t);
        type_ptr += particle_number_vect[t];
    }

    // --- per‑particle interaction matrix (n_tot × n_tot) ---
    this->interactions  = new float[n_tot * n_tot];
    this->half_strength = new float[n_tot * n_tot];

    use_fast_pow = true;
    int  first_k   = 0;
    bool first_set = false;

    for (int i = 0; i < n_tot; ++i) {
        for (int j = 0; j < n_tot; ++j) {
            float K = int_mat[types[i] * n_types + types[j]];
            float s = 2.0f * K;
            interactions[i * n_tot + j] = s;

            float hs = 0.5f * s;
            half_strength[i * n_tot + j] = hs;

            float rounded = std::round(hs * 2.0f);
            if (std::abs(hs * 2.0f - rounded) > 1e-6f)
                use_fast_pow = false;

            if (!first_set) {
                first_k   = static_cast<int>(rounded);
                first_set = true;
            } else if (static_cast<int>(rounded) != first_k) {
                first_k = -1;
            }
        }
    }
    fast_pow_k = first_k;

    // --- quasiholes ---
    if (n_holes > 0 && hole_positions != nullptr && hole_charges != nullptr) {
        this->hole_x = new float[n_holes];
        this->hole_y = new float[n_holes];
        this->hole_k = new int[n_holes];
        for (int h = 0; h < n_holes; ++h) {
            this->hole_x[h] = hole_positions[2 * h];
            this->hole_y[h] = hole_positions[2 * h + 1];
            float charge = hole_charges[h];
            int   k      = static_cast<int>(std::round(2.0f * charge));
            if (std::abs(2.0f * charge - k) > 1e-6f)
                use_fast_pow = false;   // hole charge not half‑integer
            this->hole_k[h] = k;
        }
    } else {
        this->hole_x = nullptr;
        this->hole_y = nullptr;
        this->hole_k = nullptr;
        this->n_holes = 0;
    }

    // --- positions ---
    this->positions = new float[2 * n_tot];
    for (int i = 0; i < n_tot; i++) {
        positions[2 * i]     = rand_pm(gen()) * boundary;
        positions[2 * i + 1] = rand_pm(gen()) * boundary;
    }

    // --- histogram ---
    this->histogram = new int[bins * bins * n_types];
    std::fill(histogram, histogram + bins * bins * n_types, 0);

    // --- pair‑distance cache r²_ij ---
    this->pair_r2 = new float[n_tot * n_tot];
    this->temp_r2 = new float[n_tot];

    for (int i = 0; i < n_tot; ++i) {
        pair_r2[i * n_tot + i] = 0.0f;
        for (int j = i + 1; j < n_tot; ++j) {
            float dx = positions[2*i] - positions[2*j];
            float dy = positions[2*i+1] - positions[2*j+1];
            float r2 = dx*dx + dy*dy;
            pair_r2[i * n_tot + j] = r2;
            pair_r2[j * n_tot + i] = r2;
        }
    }
}

// ── destructor ───────────────────────────────────────────────────────
Laughlin::~Laughlin()
{
    delete[] this->positions;
    delete[] this->interactions;
    delete[] this->histogram;
    delete[] this->pair_r2;
    delete[] this->half_strength;
    delete[] this->temp_r2;
    delete[] this->types;
    delete[] this->hole_x;
    delete[] this->hole_y;
    delete[] this->hole_k;
}

// ── update ───────────────────────────────────────────────────────────
void Laughlin::update(int i)
{
    float x_old = positions[2*i];
    float y_old = positions[2*i+1];
    float self_old = x_old*x_old + y_old*y_old;

    float x_new = x_old + rand_pm(gen()) * resolution;
    float y_new = y_old + rand_pm(gen()) * resolution;
    float self_new = x_new*x_new + y_new*y_new;

    float accept = std::exp(-0.5f * (self_new - self_old));

    // ── particle‑particle interaction ──
    if (use_fast_pow && fast_pow_k >= 0) {
        int k = fast_pow_k;
        for (int j = 0; j < n_tot; ++j) {
            if (j == i) continue;
            float dx = x_new - positions[2*j];
            float dy = y_new - positions[2*j+1];
            float r2_new = dx*dx + dy*dy;
            float r2_old = pair_r2[i * n_tot + j];
            float rat = r2_new / r2_old;
            temp_r2[j] = r2_new;
            accept    *= fast_half_int_pow(rat, k);
        }
    } else if (use_fast_pow) {
        for (int j = 0; j < n_tot; ++j) {
            if (j == i) continue;
            float dx = x_new - positions[2*j];
            float dy = y_new - positions[2*j+1];
            float r2_new = dx*dx + dy*dy;
            float r2_old = pair_r2[i * n_tot + j];
            float rat = r2_new / r2_old;
            temp_r2[j] = r2_new;
            int k = static_cast<int>(std::round(
                half_strength[i * n_tot + j] * 2.0f));
            accept *= fast_half_int_pow(rat, k);
        }
    } else {
        for (int j = 0; j < n_tot; ++j) {
            if (j == i) continue;
            float dx = x_new - positions[2*j];
            float dy = y_new - positions[2*j+1];
            float r2_new = dx*dx + dy*dy;
            float r2_old = pair_r2[i * n_tot + j];
            float rat = r2_new / r2_old;
            temp_r2[j] = r2_new;
            accept *= std::pow(rat, half_strength[i * n_tot + j]);
        }
    }

    // ── quasihole interaction ──
    // |ψ|² contribution of hole h:  |z_i − η_h|^{2 p_h}  =  (r²_ih)^{p_h}
    // Metropolis ratio for move z_i → z_i':
    //   (r²_new_ih / r²_old_ih)^{p_h}
    // Exponent p_h is the hole charge; precomputed k = round(2 p_h).
    for (int h = 0; h < n_holes; ++h) {
        float dx_old = x_old - hole_x[h];
        float dy_old = y_old - hole_y[h];
        float r2_old = dx_old*dx_old + dy_old*dy_old;

        float dx_new = x_new - hole_x[h];
        float dy_new = y_new - hole_y[h];
        float r2_new = dx_new*dx_new + dy_new*dy_new;

        float rat = r2_new / r2_old;
        accept *= fast_half_int_pow(rat, hole_k[h]);
    }

    // ── Metropolis acceptance ──
    if (accept > rand_01(gen())) {
        positions[2*i]   = x_new;
        positions[2*i+1] = y_new;
        for (int j = 0; j < n_tot; ++j) {
            if (j == i) continue;
            float r2 = temp_r2[j];
            pair_r2[i * n_tot + j] = r2;
            pair_r2[j * n_tot + i] = r2;
        }
    }
}

// ── record ───────────────────────────────────────────────────────────
void Laughlin::record(void)
{
    for (int i = 0; i < n_tot; i++) {
        int x_bin = static_cast<int>((positions[2*i] + boundary) / resolution);
        int y_bin = static_cast<int>((positions[2*i+1] + boundary) / resolution);
        if (x_bin >= 0 && x_bin < bins && y_bin >= 0 && y_bin < bins) {
            histogram[types[i] * bins * bins + y_bin * bins + x_bin]++;
        }
    }
}

// ── run ──────────────────────────────────────────────────────────────
void Laughlin::run(int steps)
{
    for (int step = 0; step < steps; ++step) {
        for (int k = 0; k < 5; ++k)
            for (int i = 0; i < n_tot; ++i)
                update(i);
        record();
    }
}
