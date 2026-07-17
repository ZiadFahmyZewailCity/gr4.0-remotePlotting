#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_slider.hpp>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace dashboardBlocks = gr::dashboard_blocks;

int main() {
    gr::Graph graph;

    /*
    We need 5 blocks for this decoupled live test:
    - Source: Generates the base sine wave
    - Throttle: Keeps the CPU from crashing the browser by limiting throughput
    - dep_imGUI_timeSeries: Captures DSP frames and broadcasts them to ZMQ Port 5555
    - dep_imGUI_slider: Subscribes to ZMQ Port 5556 for UI slider clicks
    - NullSink: Drains the slider block so the GR4 scheduler actively polls it
    */

    // 1. Single generator
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = 50.0f; 
    source.amplitude   = 1.0f;
    source.offset      = 0.0f;
    source.phase       = 0.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Sin; 
    source.chunk_size  = 1024;

    // 2. Throttling the source 
    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f; 
    throttle.busy_wait = false;           

    // 3. Downlink Telemetry Streamer 
    auto& ts_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    ts_sink.topic_id = "plot_1";

    // 4. Uplink Command Listener 
    auto& slider_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_slider<float>>();
    slider_src.widget_id = "slider_freq";

    // 5. Dummy drain
    auto& drain = graph.emplaceBlock<testing::NullSink<float>>();


    // Updating frequency of sine wave when slider moves on Web UI
    slider_src.on_val_update = [&source](float new_freq) {
        gr::property_map old_props;
        old_props["frequency"] = source.frequency;
        
        gr::property_map new_props;
        new_props["frequency"] = new_freq;

        source.frequency = new_freq;
        source.settingsChanged(old_props, new_props);

        std::cout << "[Coordinator] Frequency shifted to: " << new_freq << " Hz" << std::endl;
    };


    /*
    Connect Downlink: Source -> Throttle -> dep_imGUI_timeSeries
    */
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }

    auto throttle_to_sink = graph.connect<"out", "in">(throttle, ts_sink);    
    if (!throttle_to_sink.has_value()) { return 1; }

    /*
    Connect Uplink: dep_imGUI_slider -> NullSink
    */
    auto slider_to_drain = graph.connect<"out", "in">(slider_src, drain);
    if (!slider_to_drain.has_value()) { return 1; }


    /*
    Pass the graph to the scheduler and run indefinitely.
    */
    std::cout << "Starting GNU Radio 4.0 Decoupled Flowgraph Coordinator..." << std::endl;
    gr::scheduler::Simple<> scheduler;

    if (!scheduler.exchange(std::move(graph)).has_value()) {
        std::cerr << "Flowgraph failed to start!" << std::endl;
        return 1;
    }

    scheduler.runAndWait();

    return 0;
}