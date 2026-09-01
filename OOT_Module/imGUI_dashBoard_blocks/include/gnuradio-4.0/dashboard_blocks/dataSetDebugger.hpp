#ifndef GNURADIO_DASHBOARDBLOCKS_DATASETDEBUGGER_HPP
#define GNURADIO_DASHBOARDBLOCKS_DATASETDEBUGGER_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/DataSet.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <iostream>
#include <tuple>

namespace gr::debugging {

// AI GENERATED BLOCK FOR TESTING DATASET GENERATION
template <typename T>
struct DataSetDebugger : public gr::Block<DataSetDebugger<T>> {
    gr::PortIn<gr::DataSet<T>> in;

    GR_MAKE_REFLECTABLE(DataSetDebugger,in);

    [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
        for (size_t i = 0; i < input.size(); ++i) {
            const auto& ds = input[i];
            std::cout << "[DEBUG-DataSetDebugger] StreamToDataSet successful! Tag triggered. "
                      << "Captured " << ds.signal_values.size() << " samples.\n";
        }
        std::ignore = input.consume(input.size());
        return gr::work::Status::OK;
    }
};

} // namespace gr::debugging

#endif // GNURADIO_DASHBOARDBLOCKS_DATASETDEBUGGER_HPP