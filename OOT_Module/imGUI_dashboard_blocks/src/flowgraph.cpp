#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp>
#include <gnuradio-4.0/basic/StreamToDataSet.hpp>
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/insertTag.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_waterFallSink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_vectorSink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_constellationSink.hpp>
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
    constexpr float FREQ_CH2  = 1000.0f;   
    constexpr float FREQ_CH3  = 3000.0f;   

    /*
    ============================================================
    Real-valued chain shared by waterFallSink and timeSeries[cite: 11]
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

    auto& source3 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source3.sample_rate = 48000.f;
    source3.frequency   = FREQ_CH3;
    source3.amplitude   = 0.3f;
    source3.offset      = 0.0f;
    source3.phase       = 0.0f;
    source3.signal_type = basicBlocks::signal_generator::Type::Sin;
    source3.chunk_size  = 1024;

    auto& time_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    time_sink.dataSources = std::vector<std::string>{"wave_toggle", "wave_1khz", "wave_3khz"};
    time_sink.settingsChanged({}, {{"data_sources", true}});
    time_sink.id   = "time_plot_1";
    time_sink.panel_name = "Time Domain";
    time_sink.title = "Real Time Series (Float)";
    time_sink.x_axis_label = "Samples";
    time_sink.y_axis_label = "Amplitude";

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

    /*
    ============================================================
    vector_sink's dedicated sources[cite: 11]
    ============================================================
    */
    auto& vec_source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    vec_source.sample_rate = 48000.f;
    vec_source.frequency   = FREQ_LOW;
    vec_source.amplitude   = 1.0f;
    vec_source.offset      = 0.0f;
    vec_source.phase       = 0.0f;
    vec_source.signal_type = basicBlocks::signal_generator::Type::Sin;
    vec_source.chunk_size  = 1024;

    auto& vec_source2 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    vec_source2.sample_rate = 48000.f;
    vec_source2.frequency   = FREQ_CH2;
    vec_source2.amplitude   = 0.6f;
    vec_source2.offset      = 0.0f;
    vec_source2.phase       = 0.0f;
    vec_source2.signal_type = basicBlocks::signal_generator::Type::Sin;
    vec_source2.chunk_size  = 1024;

    auto& vec_source3 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    vec_source3.sample_rate = 48000.f;
    vec_source3.frequency   = FREQ_CH3;
    vec_source3.amplitude   = 0.3f;
    vec_source3.offset      = 0.0f;
    vec_source3.phase       = 0.0f;
    vec_source3.signal_type = basicBlocks::signal_generator::Type::Sin;
    vec_source3.chunk_size  = 1024;

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

    auto& vector_sink = graph.emplaceBlock<dashboardBlocks::imGUI_vectorSink<float>>();
    vector_sink.dataSources = std::vector<std::string>{"vec_toggle", "vec_1khz", "vec_3khz"};
    vector_sink.settingsChanged({}, {{"data_sources", true}});
    vector_sink.title = "check_vector_title";
    vector_sink.sink_id = "vector_ID";
    vector_sink.x_axis_label = "check_x_vector";
    vector_sink.y_axis_label = "check_y_vector";
    vector_sink.panel_name = "Vector";
    vector_sink.vectorSize = 1024;

    /*
    ============================================================
    Complex chain: drives constellationSink & time_sink_complex[cite: 11]
    ============================================================
    */
    auto& source_complex = graph.emplaceBlock<basicBlocks::SignalGenerator<std::complex<float>>>();
    source_complex.sample_rate = 48000.f;
    source_complex.frequency   = 500.0f;
    source_complex.amplitude   = 2.0f;
    source_complex.offset      = 50.0f;
    source_complex.phase       = 0.0f;
    source_complex.signal_type = basicBlocks::signal_generator::Type::Sin;
    source_complex.chunk_size  = 1024;

    auto& throttle_complex = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle_complex.target_throughput = 48000.f;
    throttle_complex.busy_wait = false;
    auto& throttle_complex_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>();

    auto& source_complex2 = graph.emplaceBlock<basicBlocks::SignalGenerator<std::complex<float>>>();
    source_complex2.sample_rate = 48000.f;
    source_complex2.frequency   = 750.0f;
    source_complex2.amplitude   = 30.0f;
    source_complex2.offset      = 0.0f;
    source_complex2.phase       = 0.0f;
    source_complex2.signal_type = basicBlocks::signal_generator::Type::Sin;
    source_complex2.chunk_size  = 1024;

    auto& throttle_complex2 = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle_complex2.target_throughput = 48000.f;
    throttle_complex2.busy_wait = false;
    auto& throttle_complex2_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>();

    auto& source_complex3 = graph.emplaceBlock<basicBlocks::SignalGenerator<std::complex<float>>>();
    source_complex3.sample_rate = 48000.f;
    source_complex3.frequency   = 1000.0f;
    source_complex3.amplitude   = 100.0f;
    source_complex3.offset      = 100.0f;
    source_complex3.phase       = 0.0f;
    source_complex3.signal_type = basicBlocks::signal_generator::Type::Sin;
    source_complex3.chunk_size  = 1024;

    auto& throttle_complex3 = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle_complex3.target_throughput = 48000.f;
    throttle_complex3.busy_wait = false;
    auto& throttle_complex3_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>();

    auto& constellation_sink = graph.emplaceBlock<dashboardBlocks::imGUI_constellationSink<std::complex<float>>>();
    constellation_sink.dataSources = std::vector<std::string>{"complex_1", "complex_2", "complex_3"};
    constellation_sink.settingsChanged({}, {{"data_sources", true}});
    constellation_sink.id = "constellation_1";
    constellation_sink.title      = "checking_title_constellation";
    constellation_sink.panel_name = "Constellation";
    constellation_sink.x_axis_label = "checking_x_axis_constellation";
    constellation_sink.y_axis_label = "checking_y_axis_constellation";
    constellation_sink.numberOfPoints = 256;

    // New Complex Time Series Sink
    auto& time_sink_complex = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<std::complex<float>>>();
    time_sink_complex.dataSources = std::vector<std::string>{"complex_time_1", "complex_time_2", "complex_time_3"};
    time_sink_complex.settingsChanged({}, {{"data_sources", true}});
    time_sink_complex.id = "time_plot_complex";
    time_sink_complex.panel_name = "Time Domain";
    time_sink_complex.title = "Complex Time Series (I/Q Split)";
    time_sink_complex.x_axis_label = "Samples";
    time_sink_complex.y_axis_label = "Amplitude";

    /*
    ============================================================
    Widgets[cite: 11]
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

    text_box.on_val_update = [&source_complex](std::string msg) {
        try {
            float new_offset = std::stof(msg);

            gr::property_map old_props;
            old_props["offset"] = source_complex.offset;

            gr::property_map new_props;
            new_props["offset"] = new_offset;

            source_complex.offset = new_offset;
            source_complex.settingsChanged(old_props, new_props);

            std::cout << "[Coordinator] Constellation offset shifted to: " << new_offset << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[TextBox] Invalid offset input: '" << msg << "' (" << e.what() << ")\n";
        }
    };

    freq_label.get_external_val = [&source]() -> std::string {
        std::ostringstream oss;
        oss << "Current Frequency: " << std::fixed << std::setprecision(2) << source.frequency << " Hz";
        return oss.str();
    };

    /*
    ============================================================
    Connections[cite: 11]
    ============================================================
    */
    auto source_to_time0 = graph.connect(source, "out", time_sink, "in#0");
    if (!source_to_time0.has_value()) { return 1; }
    auto source_to_time1 = graph.connect(source2, "out", time_sink, "in#1");
    if (!source_to_time1.has_value()) { return 1; }
    auto source_to_time2 = graph.connect(source3, "out", time_sink, "in#2");
    if (!source_to_time2.has_value()) { return 1; }

    auto source_to_waterfall = graph.connect(source, "out", waterfall_sink, "in#0");
    if (!source_to_waterfall.has_value()) { return 1; }

    auto vecSource_to_tagger = graph.connect<"out", "in">(vec_source, tagger);
    if (!vecSource_to_tagger.has_value()) { return 1; }
    auto tagger_to_s2ds = graph.connect<"out", "in">(tagger, s2ds);
    if (!tagger_to_s2ds.has_value()) { return 1; }
    auto s2ds_to_vector = graph.connect(s2ds, "out", vector_sink, "in#0");
    if (!s2ds_to_vector.has_value()) { return 1; }

    auto vecSource2_to_tagger2 = graph.connect<"out", "in">(vec_source2, tagger2);
    if (!vecSource2_to_tagger2.has_value()) { return 1; }
    auto tagger2_to_s2ds2 = graph.connect<"out", "in">(tagger2, s2ds2);
    if (!tagger2_to_s2ds2.has_value()) { return 1; }
    auto s2ds2_to_vector = graph.connect(s2ds2, "out", vector_sink, "in#1");
    if (!s2ds2_to_vector.has_value()) { return 1; }

    auto vecSource3_to_tagger3 = graph.connect<"out", "in">(vec_source3, tagger3);
    if (!vecSource3_to_tagger3.has_value()) { return 1; }
    auto tagger3_to_s2ds3 = graph.connect<"out", "in">(tagger3, s2ds3);
    if (!tagger3_to_s2ds3.has_value()) { return 1; }
    auto s2ds3_to_vector = graph.connect(s2ds3, "out", vector_sink, "in#2");
    if (!s2ds3_to_vector.has_value()) { return 1; }

    // Complex chain connections: Constellation & Complex Time Series
    auto sourceC_to_throttleC = graph.connect<"out", "in">(source_complex, throttle_complex);
    if (!sourceC_to_throttleC.has_value()) { return 1; }
    auto throttleC_to_drain = graph.connect<"out", "in">(throttle_complex, throttle_complex_drain);
    if (!throttleC_to_drain.has_value()) { return 1; }
    auto sourceC_to_constellation = graph.connect(source_complex, "out", constellation_sink, "in#0");
    if (!sourceC_to_constellation.has_value()) { return 1; }
    auto sourceC_to_timeC0 = graph.connect(source_complex, "out", time_sink_complex, "in#0");
    if (!sourceC_to_timeC0.has_value()) { return 1; }

    auto sourceC2_to_throttleC2 = graph.connect<"out", "in">(source_complex2, throttle_complex2);
    if (!sourceC2_to_throttleC2.has_value()) { return 1; }
    auto throttleC2_to_drain = graph.connect<"out", "in">(throttle_complex2, throttle_complex2_drain);
    if (!throttleC2_to_drain.has_value()) { return 1; }
    auto sourceC2_to_constellation = graph.connect(source_complex2, "out", constellation_sink, "in#1");
    if (!sourceC2_to_constellation.has_value()) { return 1; }
    auto sourceC2_to_timeC1 = graph.connect(source_complex2, "out", time_sink_complex, "in#1");
    if (!sourceC2_to_timeC1.has_value()) { return 1; }

    auto sourceC3_to_throttleC3 = graph.connect<"out", "in">(source_complex3, throttle_complex3);
    if (!sourceC3_to_throttleC3.has_value()) { return 1; }
    auto throttleC3_to_drain = graph.connect<"out", "in">(throttle_complex3, throttle_complex3_drain);
    if (!throttleC3_to_drain.has_value()) { return 1; }
    auto sourceC3_to_constellation = graph.connect(source_complex3, "out", constellation_sink, "in#2");
    if (!sourceC3_to_constellation.has_value()) { return 1; }
    auto sourceC3_to_timeC2 = graph.connect(source_complex3, "out", time_sink_complex, "in#2");
    if (!sourceC3_to_timeC2.has_value()) { return 1; }

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

    gr::dashboard_blocks::imGUI_DashboardRegistry::getInstance().set_stop_callback([&scheduler]() {
        std::ignore = scheduler.changeStateTo(gr::lifecycle::REQUESTED_STOP);
    });

    scheduler.runAndWait();

    return 0;
}