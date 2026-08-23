#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp>
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_frequencySink.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_waterFallSink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_vectorSink.hpp>
#include <gnuradio-4.0/basic/StreamToDataSet.hpp>
#include <gnuradio-4.0/dashboard_blocks/insertTag.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_checkBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_button.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_dropDownMenu.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textLabel.hpp>
#include <sstream>
#include <iomanip>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace dashboardBlocks = gr::dashboard_blocks;
namespace custom_testing = gr::custom_testing;

int main() {
    gr::Graph graph;

    constexpr float FREQ_LOW  = 2343.75f;
    constexpr float FREQ_HIGH = 4687.5f;

    /*
    ============================================================
    ISOLATION TEST: only freq_sink + widgets in this flowgraph, no other sinks,
    no other generators, no throttle/tagger/StreamToDataSet chains - stripped down
    to rule out any interference from the rest of the graph while freq_sink's
    scheduling issue is still unresolved.
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

    // Second and third sources for the multi-source test - still isolated from every
    // other sink, no throttle/drain paths needed since nothing else consumes them.
    auto& source2 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source2.sample_rate = 48000.f;
    source2.frequency   = 1000.0f;
    source2.amplitude   = 0.6f;
    source2.offset      = 0.0f;
    source2.phase       = 0.0f;
    source2.signal_type = basicBlocks::signal_generator::Type::Sin;
    source2.chunk_size  = 1024;

    auto& source3 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source3.sample_rate = 48000.f;
    source3.frequency   = 3000.0f;
    source3.amplitude   = 0.3f;
    source3.offset      = 0.0f;
    source3.phase       = 0.0f;
    source3.signal_type = basicBlocks::signal_generator::Type::Sin;
    source3.chunk_size  = 1024;

    auto& freq_sink = graph.emplaceBlock<dashboardBlocks::imGUI_frequencySink<float>>();
    freq_sink.dataSources = std::vector<std::string>{"channel_1", "channel_2", "channel_3"};
    freq_sink.settingsChanged({}, {{"data_sources", true}});
    freq_sink.id = "freq_sink";
    freq_sink.title = "Testing the title of the freq block";
    freq_sink.panel_name = "Frequency Domain";
    freq_sink.x_axis_label = "check_x_frequency";
    freq_sink.y_axis_label = "check_y_frequency";
    freq_sink.sampleRate = 48000.0f;
    freq_sink.windowSize = 1024;

    // STEP 1 of staged reintroduction: time_sink added back first, since it already
    // proved to work correctly with this exact 3-source join before freq_sink's
    // scheduling issue was found - lowest-risk block to add back first.
    auto& time_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    time_sink.dataSources = std::vector<std::string>{"wave_toggle", "wave_1khz", "wave_3khz"};
    time_sink.settingsChanged({}, {{"data_sources", true}});
    time_sink.id   = "time_plot_1";
    time_sink.panel_name = "Time Domain";
    time_sink.title = "Testing the title of the time block";
    time_sink.x_axis_label = "Checking X axis works (#59)";
    time_sink.y_axis_label = "Checking y axis works (@23)";

    // STEP 2 of staged reintroduction: waterfall_sink added back next, single source only
    // (matching the config that already proved working before the reintroduction started).
    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    waterfall_sink.settingsChanged({}, {{"data_sources", true}});
    waterfall_sink.sink_id = "waterfall_id";
    waterfall_sink.title        = "check_waterfall_title";
    waterfall_sink.x_axis_label = "check_x-axis label";
    waterfall_sink.y_axis_label = "check_y-axis label";
    waterfall_sink.panel_name   = "Frequency Domain";
    waterfall_sink.sampleRate   = 48000.0f;
    waterfall_sink.windowSize   = 1024;
    waterfall_sink.history_size = 100;

    // STEP 3 of staged reintroduction: vector_sink added back next, along with its
    // required tag/StreamToDataSet framing chains (one per source, since it consumes
    // DataSet<T> rather than a plain stream).
    auto& tagger = graph.emplaceBlock<custom_testing::insertTag<float>>();
    tagger.interval = 1024;
    tagger.offset   = 0;
    tagger.tag_key  = "start";

    auto& s2ds = graph.emplaceBlock<basicBlocks::StreamToDataSet<float>>();
    s2ds.filter = "[start/, start/]";
    s2ds.n_max  = 1024;
    s2ds.n_pre  = 0;
    s2ds.n_post = 1024;

    auto& tagger2 = graph.emplaceBlock<custom_testing::insertTag<float>>();
    tagger2.interval = 1024;
    tagger2.offset   = 0;
    tagger2.tag_key  = "start";

    auto& s2ds2 = graph.emplaceBlock<basicBlocks::StreamToDataSet<float>>();
    s2ds2.filter = "[start/, start/]";
    s2ds2.n_max  = 1024;
    s2ds2.n_pre  = 0;
    s2ds2.n_post = 1024;

    auto& tagger3 = graph.emplaceBlock<custom_testing::insertTag<float>>();
    tagger3.interval = 1024;
    tagger3.offset   = 0;
    tagger3.tag_key  = "start";

    auto& s2ds3 = graph.emplaceBlock<basicBlocks::StreamToDataSet<float>>();
    s2ds3.filter = "[start/, start/]";
    s2ds3.n_max  = 1024;
    s2ds3.n_pre  = 0;
    s2ds3.n_post = 1024;

    // ISOLATION TEST: vector_sink itself removed entirely for this test (not just
    // disconnected) - a block with unconnected required ports could itself cause a
    // scheduler error, muddying the result. s2ds outputs go to NullSinks instead, to
    // test whether the tag/StreamToDataSet framing pipeline itself stalls the shared
    // source buffers independent of what consumes its output, or whether it's
    // specifically vector_sink's own processBulk that's the problem.
    auto& s2ds_drain  = graph.emplaceBlock<testing::NullSink<gr::DataSet<float>>>();
    auto& s2ds2_drain = graph.emplaceBlock<testing::NullSink<gr::DataSet<float>>>();
    auto& s2ds3_drain = graph.emplaceBlock<testing::NullSink<gr::DataSet<float>>>();

    /*
    ============================================================
    Widgets: all on the "Controls" panel. Checkbox, button, and dropdown
    all drive the same shared toggle between FREQ_LOW/FREQ_HIGH on the real source.
    Text box just logs its input (no complex source in this stripped graph to drive).
    Label reports the real frequency.
    ============================================================
    */
    auto& checkBox_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_checkBox<float>>();
    checkBox_src.widget_id  = "freq_checkBox";
    checkBox_src.panel_name = "Controls";

    auto& button_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_button<float>>();
    button_src.widget_id  = "freq_button";
    button_src.panel_name = "Controls";

    auto& checkBox_drain = graph.emplaceBlock<testing::NullSink<float>>();
    auto& button_drain   = graph.emplaceBlock<testing::NullSink<float>>();

    auto& dropdown_src = graph.emplaceBlock<dashboardBlocks::imGUI_dropDownMenu<float>>();
    dropdown_src.widget_id     = "freq_dropdown";
    dropdown_src.panel_name    = "Controls";
    dropdown_src.target_property = "frequency";
    dropdown_src.options       = std::vector<std::string>{"Low", "High"};
    auto& dropdown_drain = graph.emplaceBlock<testing::NullSink<float>>();

    auto& text_box = graph.emplaceBlock<dashboardBlocks::imGUI_textBox<float>>();
    text_box.widget_id  = "offset_input_1";
    text_box.panel_name = "Controls";
    auto& text_drain = graph.emplaceBlock<testing::NullSink<float>>();

    auto& freq_label = graph.emplaceBlock<dashboardBlocks::imGUI_textLabel<float>>();
    freq_label.widget_id  = "freq_label";
    freq_label.panel_name = "Controls";
    auto& freq_label_drain = graph.emplaceBlock<testing::NullSink<float>>();

    // Shared toggle state driving the real source's frequency
    bool freq_toggle_state = false;

    auto apply_freq_toggle = [&source](bool state) {
        gr::property_map old_props;
        old_props["frequency"] = source.frequency;

        float new_freq = state ? FREQ_HIGH : FREQ_LOW;
        gr::property_map new_props;
        new_props["frequency"] = new_freq;

        source.frequency = new_freq;
        source.settingsChanged(old_props, new_props);

        std::cout << "[Coordinator] Frequency shifted to: " << new_freq << " Hz" << std::endl;
    };

    checkBox_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](bool new_state) {
        freq_toggle_state = new_state;
        apply_freq_toggle(freq_toggle_state);
    };
    checkBox_src.get_external_val = [&freq_toggle_state]() -> bool {
        return freq_toggle_state;
    };

    button_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](bool) {
        freq_toggle_state = !freq_toggle_state;
        apply_freq_toggle(freq_toggle_state);
    };

    dropdown_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](std::string selected_option) {
        freq_toggle_state = (selected_option == "High");
        apply_freq_toggle(freq_toggle_state);
    };
    dropdown_src.get_external_val = [&freq_toggle_state]() -> std::string {
        return freq_toggle_state ? "High" : "Low";
    };

    // No complex source in this stripped graph anymore - text box just logs its input
    // instead of driving a constellation offset.
    text_box.on_val_update = [](std::string msg) {
        std::cout << "[Coordinator] Text box input received: '" << msg << "'" << std::endl;
    };

    freq_label.get_external_val = [&source]() -> std::string {
        std::ostringstream oss;
        oss << "Current Frequency: " << std::fixed << std::setprecision(2) << source.frequency << " Hz";
        return oss.str();
    };

    /*
    ============================================================
    Connections
    ============================================================
    */
    auto source_to_freq0 = graph.connect(source, "out", freq_sink, "in#0");
    if (!source_to_freq0.has_value()) { return 1; }
    auto source_to_freq1 = graph.connect(source2, "out", freq_sink, "in#1");
    if (!source_to_freq1.has_value()) { return 1; }
    auto source_to_freq2 = graph.connect(source3, "out", freq_sink, "in#2");
    if (!source_to_freq2.has_value()) { return 1; }

    auto source_to_time0 = graph.connect(source, "out", time_sink, "in#0");
    if (!source_to_time0.has_value()) { return 1; }
    auto source_to_time1 = graph.connect(source2, "out", time_sink, "in#1");
    if (!source_to_time1.has_value()) { return 1; }
    auto source_to_time2 = graph.connect(source3, "out", time_sink, "in#2");
    if (!source_to_time2.has_value()) { return 1; }

    auto source_to_waterfall = graph.connect(source, "out", waterfall_sink, "in#0");
    if (!source_to_waterfall.has_value()) { return 1; }

    auto source_to_tagger = graph.connect<"out", "in">(source, tagger);
    if (!source_to_tagger.has_value()) { return 1; }
    auto tagger_to_s2ds = graph.connect<"out", "in">(tagger, s2ds);
    if (!tagger_to_s2ds.has_value()) { return 1; }
    auto s2ds_to_drain = graph.connect<"out", "in">(s2ds, s2ds_drain);
    if (!s2ds_to_drain.has_value()) { return 1; }

    auto source2_to_tagger2 = graph.connect<"out", "in">(source2, tagger2);
    if (!source2_to_tagger2.has_value()) { return 1; }
    auto tagger2_to_s2ds2 = graph.connect<"out", "in">(tagger2, s2ds2);
    if (!tagger2_to_s2ds2.has_value()) { return 1; }
    auto s2ds2_to_drain = graph.connect<"out", "in">(s2ds2, s2ds2_drain);
    if (!s2ds2_to_drain.has_value()) { return 1; }

    auto source3_to_tagger3 = graph.connect<"out", "in">(source3, tagger3);
    if (!source3_to_tagger3.has_value()) { return 1; }
    auto tagger3_to_s2ds3 = graph.connect<"out", "in">(tagger3, s2ds3);
    if (!tagger3_to_s2ds3.has_value()) { return 1; }
    auto s2ds3_to_drain = graph.connect<"out", "in">(s2ds3, s2ds3_drain);
    if (!s2ds3_to_drain.has_value()) { return 1; }

    // Widget uplinks -> NullSinks
    auto checkBox_to_drain = graph.connect<"out", "in">(checkBox_src, checkBox_drain);
    if (!checkBox_to_drain.has_value()) { return 1; }

    auto button_to_drain = graph.connect<"out", "in">(button_src, button_drain);
    if (!button_to_drain.has_value()) { return 1; }

    auto dropdown_to_drain = graph.connect<"out", "in">(dropdown_src, dropdown_drain);
    if (!dropdown_to_drain.has_value()) { return 1; }

    auto text_to_drain = graph.connect<"out", "in">(text_box, text_drain);
    if (!text_to_drain.has_value()) { return 1; }

    auto freq_label_to_drain = graph.connect<"out", "in">(freq_label, freq_label_drain);
    if (!freq_label_to_drain.has_value()) { return 1; }

    std::cout << "Starting GNU Radio 4.0 Decoupled Flowgraph Coordinator..." << std::endl;
    gr::scheduler::Simple<> scheduler;

    if (!scheduler.exchange(std::move(graph)).has_value()) {
        std::cerr << "Flowgraph failed to start!" << std::endl;
        return 1;
    }

    //If the flowgraph will be killed using a SIGINT call, then you have to attach the scheduler to this lambda or define your own handler
    gr::dashboard_blocks::imGUI_DashboardRegistry::getInstance().set_stop_callback([&scheduler]() {
        std::ignore = scheduler.changeStateTo(gr::lifecycle::REQUESTED_STOP);
    });

    scheduler.runAndWait();

    return 0;
}