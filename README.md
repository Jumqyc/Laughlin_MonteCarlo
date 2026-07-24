# Laughlin Monte Carlo

Monte Carlo simulation of Laughlin and Halperin fractional quantum Hall
states.  Computes real‑space density profiles via Metropolis sampling of
$|\psi|^2$.

## Quick start

```bash
python -m venv .venv && source .venv/bin/activate
pip install numpy matplotlib
./build.sh
```
Now the python modules are in the root folder and you can play with it in demo.ipynb.

## Physics

The code samples $|\psi|^2$ for an **arbitrary number of species** $T$ with a
generic symmetric interaction matrix $K_{ab}$: $\psi(\{z_i\}) = \prod_{i<j} (z_i - z_j)^{K_{t_i,t_j}} \exp\left(-\tfrac{1}{4}\sum_k |z_k|^2\right)$ and the modular square is $|\psi|^2 \propto\exp\left(-\tfrac{1}{2}\sum_k |z_k|^2\right)\prod_{i<j} |z_i - z_j|^{2K_{t_i,t_j}}$

$t_i \in \{0, \dots, T-1\}$ labels the species of particle $i$.

**Important special cases:**

* **Laughlin** ($T = 1$, $K = [m]$) — the $\nu = 1/m$ state ($m$ odd).
* **Halperin $(m, m, n)$** ($T = 2$) — two‑species state with
  $K = \begin{bmatrix} m & n \\ n & m \end{bmatrix}$.
  $m$ controls intra‑species repulsion, $n$ the inter‑species repulsion.
  When $n < m$ the fluids interpenetrate; $n > m$ drives phase separation.
* **Quasiholes** — a quasihole at position $\eta$ adds a factor
  $\prod_i (z_i - \eta)^{p}$ to $\psi$.  This can be modelled by adding an
  extra species with a single particle and suitable $K$ entries, or by
  treating the hole as a fixed particle (not updated during the chain).
* **Multi‑component generalisation** — any $T \times T$ symmetric integer
  matrix $K$ defines a valid FQHE wavefunction, supporting $T$ distinct
  species with arbitrary mutual statistics.

## Python API

```python
import numpy as np
import Laughlin

# K-matrix for Halperin 331:  K = [[3, 1], [1, 3]]
int_mat = np.array([[3.0, 1.0],
                     [1.0, 3.0]], dtype=np.float32)

sim = Laughlin.Laughlin(int_mat,
                        np.array([15, 15], dtype=np.int32),
                        ptype_num=2,          # number of species
                        bdr=20.0,             # half-width of box  [−bdr, bdr]
                        res=0.1)              # bin width

sim.run(steps=200000)                         # run the chain

hist = sim.get_histogram()                    # shape (ptype_num, bins, bins)
bins = sim.get_bins()                         # bins per axis
```

### Constructor

```
Laughlin(int_mat, particle_numbers, ptype_num, bdr, res)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `int_mat` | `np.float32` array $(T \times T)$ | Interaction matrix that labels topological order of Abelian FQHE |
| `particle_numbers` | `np.int32` array $(T,)$ | Number of particles of each species. |
| `ptype_num` | `int` | Number of species $T$. |
| `bdr` | `float` | Box half‑width; coordinates $\in [-bdr, bdr]$. |
| `res` | `float` | Spatial resolution; total bins = $2 bdr / res + 1$. |

### Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `run(steps)` | — | Perform $5 \cdot N \cdot steps$ single‑particle Metropolis updates. |
| `get_histogram()` | `ndarray` $(T, B, B)$ | Visit counts binned on the $B \times B$ spatial grid.  Normalise by $\mathrm{sum} \cdot res^2$ to obtain a probability density. |
| `get_bins()` | `int` | Number of bins $B$ per axis. |
| `get_num_types()` | `int` | Number of species $T$. |
| `get_total_particles()` | `int` | Total particle count $\sum N_a$. |

### K‑matrix reference

The physical $K$ matrix defines $|\psi|^2 \propto \prod |z_i-z_j|^{2K_{ab}}$.
The C++ layer multiplies internally by 2, so you pass the bare $K$ matrix.

| State | $K$ | `int_mat` |
|-------|-----|-----------|
| Laughlin $m=1$ | $\begin{bmatrix}1\end{bmatrix}$ | `[[1.0]]` |
| Laughlin $m=3$ | $\begin{bmatrix}3\end{bmatrix}$ | `[[3.0]]` |
| Laughlin $m=5$ | $\begin{bmatrix}5\end{bmatrix}$ | `[[5.0]]` |
| Halperin 331 | $\begin{bmatrix}3&1\\1&3\end{bmatrix}$ | `[[3.0, 1.0], [1.0, 3.0]]` |
| Halperin 332 | $\begin{bmatrix}3&2\\2&3\end{bmatrix}$ | `[[3.0, 2.0], [2.0, 3.0]]` |

## Build

```bash
./build.sh              # Release  (−O3 −march=native −flto)
./build.sh RelWithDebInfo  # Optimised + debug symbols
./build.sh Debug        # Debug  (−O0 −g)
```

Requires `cmake` ≥ 3.14, `g++` ≥ 13, Python ≥ 3.12.  `pybind11` is bundled
in `cpp/extern/`.

## Files

| File | Purpose |
|------|---------|
| `build.sh` | Build C++ module, copy `.so` to project root |
| `cpp/Laughlin.hpp` | C++ class declaration |
| `cpp/Laughlin.cpp` | Metropolis update with pair‑distance cache |
| `cpp/bind.cpp` | pybind11 Python bindings |
| `demo.ipynb`| Some example usage