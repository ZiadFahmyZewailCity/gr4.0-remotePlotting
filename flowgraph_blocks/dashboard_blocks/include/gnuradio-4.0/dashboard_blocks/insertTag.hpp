#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/meta/utils.hpp>
#include <gnuradio-4.0/Tag.hpp> // Required for gr::tag::TRIGGER_NAME and CONTEXT
#include <iostream> 
#include <tuple> 

namespace gr::custom_testing {

// AI GENERATED BLOCK FOR ADDING A TAG EVERY N NUMBER OF SAMPLES

// Register the block for floats and complex floats
GR_REGISTER_BLOCK("gr::custom_testing::insertTag", gr::custom_testing::insertTag, ([T]), [float, std::complex<float>]);

template<typename T>
requires(std::is_arithmetic_v<T> || gr::meta::complex_like<T>)
struct insertTag : Block<insertTag<T>> {
    using Description = Doc<R"(@brief Injects a specific tag into the stream after a configurable offset, and then every 'interval' samples.)">;

    template<typename U, gr::meta::fixed_string description = "", typename... Arguments>
    using A = Annotated<U, description, Arguments...>;

    // Port definitions
    PortIn<T>  in;
    PortOut<T> out;

    // --- Configurable Settings ---
    A<uint64_t, "interval", Visible, Doc<"Number of samples between tags">> interval = 1024;
    A<uint64_t, "offset", Visible, Doc<"Number of samples to wait before the first tag">> offset = 0;
    A<std::string, "tag_key", Visible, Doc<"Name of the tag to inject">> tag_key = "start";

    // Expose settings to the GNU Radio 4.0 framework
    GR_MAKE_REFLECTABLE(insertTag, in, out, interval, offset, tag_key);

    uint64_t _total_samples = 0;

    gr::work::Status processBulk(auto& inSamples, auto& outSamples) {
        const size_t n = std::min(inSamples.size(), outSamples.size());

        std::cout << "[insertTag] processBulk called: n=" << n
                   << " _total_samples=" << _total_samples
                   << " interval=" << interval.value
                   << " offset=" << offset.value << std::endl;

        // Pass the signal data through directly
        std::ranges::copy_n(inSamples.begin(), static_cast<std::ptrdiff_t>(n), outSamples.begin());

        // Calculate if and where a tag should be placed in this chunk
        for (size_t i = 0; i < n; ++i) {
            // Protect against divide-by-zero if interval is set to 0,
            // and only tag after the initial offset is reached.
            if (interval > 0 && _total_samples >= offset && (_total_samples - offset) % interval == 0) {
                
                gr::property_map tagData;
                
                // Use canonical GR4 trigger keys instead of arbitrary keys
                tagData[gr::property_map::key_type(gr::tag::TRIGGER_NAME.shortKey())] = tag_key.value;
                tagData[gr::property_map::key_type(gr::tag::CONTEXT.shortKey())] = std::string("");

                std::cout << "[insertTag] publishing tag TRIGGER_NAME='" << tag_key.value
                           << "' at local index " << i
                           << " (absolute sample " << _total_samples << ")" << std::endl;

                outSamples.publishTag(tagData, i);
            }
            _total_samples++;
        }

        std::ignore = inSamples.consume(n);
        outSamples.publish(n); // returns void on this port type -- don't assign to std::ignore

        return gr::work::Status::OK;
    }
};

} // namespace gr::custom_testing