#ifndef GNURADIO_DASHBOARDBLOCKS_FREQUENCY_MODULATOR_HPP
#define GNURADIO_DASHBOARDBLOCKS_FREQUENCY_MODULATOR_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <iostream>
#include <functional> // Required for lambdas

//THIS IS THE AI GENERATED PURELY FOR TESTING HAVING THE VARIABLE BEING VARIED FROM WITHIN THE FLOWGRAPH TO SEE HOW IT WILL BE UPDATED IN THE DASHBOARD


namespace gr::dashboard_blocks {

    template <typename T>
    struct frequency_modulator : gr::Block<frequency_modulator<T>> {

        using Description = gr::Doc<R""(Dummy block that uses lambdas to safely periodically multiply a frequency variable.)"">;

        gr::PortIn<T> in;
        gr::PortOut<T> out;

        // Lambda functions to replace the raw pointer
        std::function<T()> get_freq = nullptr;
        std::function<void(T)> set_freq = nullptr;

        // Internal timing
        uint64_t sample_counter = 0;
        uint64_t sample_rate = 48000;
        uint64_t wait_time_seconds = 5;

        frequency_modulator(gr::property_map initial_settings = {})
            : gr::Block<frequency_modulator<T>>(initial_settings) {}

        GR_MAKE_REFLECTABLE(frequency_modulator, in, out);

        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input, gr::OutputSpanLike auto& output) {
            const std::size_t nSamples = std::min(input.size(), output.size());
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;

            // 1. Pass the data through untouched
            for (std::size_t i = 0; i < nSamples; ++i) {
                output[i] = input[i];
            }

            // 2. Advance the timer
            sample_counter += nSamples;
            if (sample_counter >= (sample_rate * wait_time_seconds)) {
                sample_counter = 0;

                // 3. Use lambdas to safely read, modify, and write the value!
                if (get_freq && set_freq) {
                    T current_val = get_freq();
                    T new_val;
                    
                    if (current_val == static_cast<T>(50.0)) {
                        new_val = current_val * static_cast<T>(2.0); // Jump to 100.0
                    } else {
                        new_val = current_val / static_cast<T>(2.0); // Drop to 50.0
                    }
                    
                    set_freq(new_val);
                    std::cout << "\n[Modulator] Lambda safely shifted frequency to: " << new_val << " Hz\n";
                }
            }

                        // To this:
            if (!input.consume(nSamples)) {
                return gr::work::Status::ERROR;
            }
            output.publish(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::frequency_modulator", gr::dashboard_blocks::frequency_modulator, [float])

#endif