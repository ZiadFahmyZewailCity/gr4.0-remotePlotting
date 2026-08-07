#ifndef GNURADIO_DASHBOARDBLOCKS_STREAMTOVECTOR_HPP
#define GNURADIO_DASHBOARDBLOCKS_STREAMTOVECTOR_HPP

#include <gnuradio-4.0/Block.hpp>
#include <vector>
#include <tuple> // Required for std::ignore

//AI GENERATE BLOCK FOR TESTING OF VECTOR SINK


namespace gr::dashboard_blocks {

template <typename T>
struct StreamToVector : public gr::Block<StreamToVector<T>> {
    using Description = gr::Doc<R""(Converts a continuous stream of items into discrete vectors)"">;

    // Input takes individual items, Output produces whole vectors
    gr::PortIn<T> in;
    gr::PortOut<std::vector<T>> out;

    gr::Annotated<size_t, "vector_size"> vector_size = 1024UL;

    GR_MAKE_REFLECTABLE(StreamToVector, in, out, vector_size);

     void start() {
        // Don't let the scheduler invoke processBulk until a full vector's
        // worth of samples is actually available — avoids the
        // INSUFFICIENT_INPUT_ITEMS livelock when vector_size > default chunk size.
        in.min_samples = vector_size.value;
    }



    [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input, gr::OutputSpanLike auto& output) {
        

        std::cout << "[s2v] input.size()=" << input.size()
              << " output.size()=" << output.size() << std::endl;

        // Calculate how many complete vectors we can mathematically make this cycle
        size_t n_vectors = std::min(input.size() / vector_size.value, output.size());

        std::cout << "[s2v] n_vectors=" << n_vectors << std::endl;
        // Block if we don't have enough data to make at least one
        if (n_vectors == 0) {
            if (input.size() < vector_size.value) {
                return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
            }
            return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;
        }

        // Process all available full vectors in bulk
        for (size_t i = 0; i < n_vectors; ++i) {
            output.data()[i] = std::vector<T>(
                input.data() + (i * vector_size.value),
                input.data() + ((i + 1) * vector_size.value)
            );
        }

        // 1. Fix the [[nodiscard]] warning by explicitly capturing the bool
        std::ignore = input.consume(n_vectors * vector_size.value);

        // 2. IMPORTANT: publish only what was actually written this cycle.
        // Without this call, GR4 assumes the *entire* span (output.size()) was
        // filled whenever we return OK. Since n_vectors is frequently smaller
        // than output.size() (plenty of output room, not enough input yet),
        // skipping this would push default-constructed / empty std::vector<T>
        // entries downstream -- which is what causes imGUI_vectorSink to publish
        // header-only (payload-less) ZMQ messages after the first real one.
        output.publish(n_vectors);

        return gr::work::Status::OK;
    }
};

} // namespace gr::dashboard_blocks

#endif