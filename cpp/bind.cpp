#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <stdexcept>
#include "Laughlin.hpp"

namespace py = pybind11;

py::array_t<int> histogram_as_numpy(Laughlin &self)
{
    int bins = self.get_bins();
    int n_types = self.get_num_types();
    int total_bins = bins * bins * n_types;
    int *data = self.get_histogram();
    auto result = py::array_t<int>(total_bins);
    py::buffer_info buf = result.request();
    std::copy(data, data + total_bins, static_cast<int *>(buf.ptr));
    result.resize({n_types, bins, bins});
    return result;
}

PYBIND11_MODULE(Laughlin, m)
{
    m.doc() = "Laughlin Monte Carlo — multi‑species FQHE with quasiholes";

    py::class_<Laughlin>(m, "Laughlin")
        // ── without quasiholes ──
        .def(py::init([](py::array_t<float> int_mat,
                         py::array_t<int> particle_numbers,
                         int ptype_num,
                         float bdr,
                         float res) {
            auto bi = int_mat.request();
            auto bp = particle_numbers.request();
            if (bi.ndim != 2 || bi.shape[0] != ptype_num ||
                bi.shape[1] != ptype_num)
                throw std::runtime_error(
                    "int_mat must be (ptype_num × ptype_num)");
            if (bp.ndim != 1 || bp.shape[0] != ptype_num)
                throw std::runtime_error(
                    "particle_numbers must be length ptype_num");
            return new Laughlin(static_cast<float *>(bi.ptr),
                                static_cast<int *>(bp.ptr),
                                ptype_num, bdr, res);
        }))

        // ── with quasiholes ──
        .def(py::init([](py::array_t<float> int_mat,
                         py::array_t<int> particle_numbers,
                         int ptype_num,
                         float bdr,
                         float res,
                         py::array_t<float> hole_positions,
                         py::array_t<float> hole_charges) {
            auto bi = int_mat.request();
            auto bp = particle_numbers.request();
            auto bh = hole_positions.request();
            auto bc = hole_charges.request();
            if (bi.ndim != 2 || bi.shape[0] != ptype_num ||
                bi.shape[1] != ptype_num)
                throw std::runtime_error(
                    "int_mat must be (ptype_num × ptype_num)");
            if (bp.ndim != 1 || bp.shape[0] != ptype_num)
                throw std::runtime_error(
                    "particle_numbers must be length ptype_num");
            if (bh.ndim != 2 || bh.shape[1] != 2)
                throw std::runtime_error(
                    "hole_positions must be (n_holes × 2)");
            int n_holes = bh.shape[0];
            if (bc.ndim != 1 || bc.shape[0] != n_holes)
                throw std::runtime_error(
                    "hole_charges must be length n_holes");
            return new Laughlin(static_cast<float *>(bi.ptr),
                                static_cast<int *>(bp.ptr),
                                ptype_num, bdr, res,
                                static_cast<float *>(bh.ptr),
                                static_cast<float *>(bc.ptr),
                                n_holes);
        }))

        .def("run", &Laughlin::run, py::arg("steps"))
        .def("get_histogram", &histogram_as_numpy)
        .def("get_bins", &Laughlin::get_bins)
        .def("get_num_types", &Laughlin::get_num_types)
        .def("get_total_particles", &Laughlin::get_total_particles)
        .def("get_num_holes", &Laughlin::get_num_holes);
}
