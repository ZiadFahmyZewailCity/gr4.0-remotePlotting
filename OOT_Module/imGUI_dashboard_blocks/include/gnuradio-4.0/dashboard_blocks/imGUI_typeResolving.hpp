#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_TYPERESOLVING_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_TYPERESOLVING_HPP

#include <string>
#include <complex>
#include <type_traits>

//TO DO: Add comments explaining the point of this file

namespace gr::dashboard_blocks {

    // Compile time type extraction
    template <typename T>
    struct extract_real { 
        using type = T; 
    };

    template <typename T>
    struct extract_real<std::complex<T>> { 
        using type = T; 
    };

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