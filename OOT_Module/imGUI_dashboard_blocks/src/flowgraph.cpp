#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/algorithm/fourier/window.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_frequencySink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_waterFallSink.hpp>
#include <iostream>

namespace basicBlocks   = gr::basic;
namespace dashboardBlocks = gr::dashboard_blocks;

int main() {
    gr::Graph graph;

    constexpr float FREQ_LOW = 2343.75f;
    constexpr float FREQ_CH2 = 1000.0f;

    /*
    ============================================================
    Sources - two channels so phase can be checked per-source,
    not just for a single trace
    ============================================================
    */
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = FREQ_LOW;
    source.amplitude   = 1.0f;
    source.offset      = 0.0f;
    source.phase       = 0.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Sin;
    source.chunk_size  = 1024;

    auto& source2 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source2.sample_rate = 48000.f;
    source2.frequency   = FREQ_CH2;
    source2.amplitude   = 0.6f;
    source2.offset      = 0.0f;
    source2.phase       = 0.0f;
    source2.signal_type = basicBlocks::signal_generator::Type::Sin;
    source2.chunk_size  = 1024;

    /*
    ============================================================
    Waterfall sink - unchanged, single source, just here so the
    dashboard has something to compare the frequency sink against
    ============================================================
    */
    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    waterfall_sink.settingsChanged({}, {{"data_sources", true}});
    waterfall_sink.id           = "waterfall_id";
    waterfall_sink.title        = "check_waterfall_title";
    waterfall_sink.x_axis_label = "check_x-axis label";
    waterfall_sink.y_axis_label = "check_y-axis label";
    waterfall_sink.panel_name   = "Frequency Domain";
    waterfall_sink.sampleRate   = 48000.0f;
    waterfall_sink.windowSize   = 1024;
    waterfall_sink.history_size = 100;

    /*
    ============================================================
    Frequency sink - this is what we're actually testing:
      - publishPhase enabled, so each source should show up twice
        in the dashboard config: "freq_1", "freq_1_phase", etc.
      - windowType deliberately set away from the Hann default
    ============================================================
    */
    auto& freq_sink = graph.emplaceBlock<dashboardBlocks::imGUI_frequencySink<float>>();
    freq_sink.dataSources = std::vector<std::string>{"freq_1", "freq_2"};
    freq_sink.settingsChanged({}, {{"data_sources", true}});
    freq_sink.id            = "freq_sink_id";
    freq_sink.title         = "Real Frequency Spectrum";
    freq_sink.x_axis_label  = "Frequency (Hz)";
    freq_sink.y_axis_label  = "Magnitude";
    freq_sink.panel_name    = "Frequency Domain";
    freq_sink.sampleRate    = 48000.0f;
    freq_sink.windowSize    = 1024;

    // Pick a window other than the Hann default
    freq_sink.windowType = gr::algorithm::window::Type::Exponential;

    // Phase output
    freq_sink.publishPhase      = false;
    freq_sink.unwrapPhase       = false;
    freq_sink.outputPhaseInDeg  = false;

    /*
    ============================================================
    Connections
    ============================================================
    */
    auto source_to_waterfall = graph.connect(source, "out", waterfall_sink, "in#0");
    if (!source_to_waterfall.has_value()) { return 1; }

    auto source_to_freq0 = graph.connect(source, "out", freq_sink, "in#0");
    if (!source_to_freq0.has_value()) { return 1; }
    auto source_to_freq1 = graph.connect(source2, "out", freq_sink, "in#1");
    if (!source_to_freq1.has_value()) { return 1; }

    std::cout << "Starting GNU Radio 4.0 Decoupled Flowgraph Coordinator..." << std::endl;
    gr::scheduler::Simple<> scheduler;

    if (!scheduler.exchange(std::move(graph)).has_value()) {
        std::cerr << "Flowgraph failed to start!" << std::endl;
        return 1;
    }

    gr::dashboard_blocks::imGUI_DashboardRegistry::getInstance().set_stop_callback([&scheduler]() {
        std::ignore = scheduler.changeStateTo(gr::lifecycle::REQUESTED_STOP);
    });

    scheduler.runAndWait();

    return 0;
}