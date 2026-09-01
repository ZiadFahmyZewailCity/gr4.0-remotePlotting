#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_TYPERESOLVING_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_TYPERESOLVING_HPP

#include <gnuradio-4.0/meta/utils.hpp>
#include <concepts>
#include <complex>
#include <string>
#include <type_traits>

//TO DO: Add comments explaining the point of this file

namespace gr::dashboard_blocks {

    // Compile time type extraction
    template <typename T>
    struct scalar_type {
        using type = T;
    };

    template <gr::meta::complex_like T>
    struct scalar_type<T> {
        using type = typename T::value_type;
    };

    template <typename T>
    using scalar_type_t = typename scalar_type<T>::type;

    // Compile-Time Type tags for the dataSources of the config file
    template <typename U>
    inline std::string dashboard_dtypeTag() {
        if constexpr (std::is_same_v<U, float>)                  { return "float32"; }
        else if constexpr (std::is_same_v<U, double>)             { return "float64"; }
        else if constexpr (std::is_same_v<U, std::complex<float>>) { return "complex64"; }
        else if constexpr (std::is_same_v<U, std::complex<double>>){ return "complex128"; }
        else                                                      { return "unknown"; }
    }

} // namespace gr::dashboard_blocks

#endif // GNURADIO_DASHBOARDBLOCKS_IMGUI_TYPERESOLVING_HPP