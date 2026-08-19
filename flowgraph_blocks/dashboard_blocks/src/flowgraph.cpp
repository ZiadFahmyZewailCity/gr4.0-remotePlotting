#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp>
#include <gnuradio-4.0/basic/StreamToDataSet.hpp>
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/insertTag.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_frequencySink.hpp>
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

    /*
    ============================================================
    Real-valued chain: drives frequencySink, waterFallSink,
    timeSeries, and vectorSink (via tag + StreamToDataSet framing,
    which vectorSink actually requires - it takes DataSet<T>, not a plain stream)
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

    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f;
    throttle.busy_wait = false;
    auto& throttle_drain = graph.emplaceBlock<testing::NullSink<float>>();

    auto& freq_sink = graph.emplaceBlock<dashboardBlocks::imGUI_frequencySink<float>>();
    freq_sink.id = "freq_sink";
    freq_sink.title = "Testing the title of the freq block";
    freq_sink.panel_name = "Frequency Domain";
    freq_sink.x_axis_label = "check_x_frequency";
    freq_sink.y_axis_label = "check_y_frequency";
    freq_sink.sampleRate = 48000.0f;
    freq_sink.windowSize = 1024;

    auto& time_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    time_sink.id   = "time_plot_1";
    time_sink.panel_name = "Time Domain";
    time_sink.title = "Testing the title of the time block";
    time_sink.x_axis_label = "Checking X axis works (#59)";
    time_sink.y_axis_label = "Checking y axis works (@23)";



    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    waterfall_sink.sink_id = "waterfall_id";
    waterfall_sink.title        = "check_waterfall_title";
    waterfall_sink.x_axis_label = "check_x-axis label";
    waterfall_sink.y_axis_label = "check_y-axis label";
    waterfall_sink.panel_name   = "Frequency Domain";
    waterfall_sink.sampleRate   = 48000.0f;
    waterfall_sink.windowSize   = 1024;
    waterfall_sink.history_size = 100;


    auto& tagger = graph.emplaceBlock<custom_testing::insertTag<float>>();
    tagger.interval = 1024;
    tagger.offset   = 0;
    tagger.tag_key  = "start";

    auto& s2ds = graph.emplaceBlock<basicBlocks::StreamToDataSet<float>>();
    s2ds.filter = "[start/, start/]";
    s2ds.n_max  = 1024;
    s2ds.n_pre  = 0;
    s2ds.n_post = 1024;

    auto& vector_sink = graph.emplaceBlock<dashboardBlocks::imGUI_vectorSink<float>>();
    vector_sink.title = "check_vector_title";
    vector_sink.sink_id = "vector_ID";
    vector_sink.x_axis_label = "check_x_vector";
    vector_sink.y_axis_label = "check_x_vector";
    vector_sink.panel_name = "Vector";
    vector_sink.vectorSize = 1024;

    /*
    ============================================================
    Complex chain: drives constellationSink (needs PortIn<complex<T>>)
    Circle centered at (50,50) radius 40 - stays inside the sink's
    default 0-100 axis bounds while leaving room to move via the offset control
    ============================================================
    */
    auto& source_complex = graph.emplaceBlock<basicBlocks::SignalGenerator<std::complex<float>>>();
    source_complex.sample_rate = 48000.f;
    source_complex.frequency   = 500.0f;
    source_complex.amplitude   = 40.0f;
    source_complex.offset      = 50.0f;
    source_complex.phase       = 0.0f;
    source_complex.signal_type = basicBlocks::signal_generator::Type::Sin;
    source_complex.chunk_size  = 1024;

    auto& throttle_complex = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle_complex.target_throughput = 48000.f;
    throttle_complex.busy_wait = false;
    auto& throttle_complex_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>();

    auto& constellation_sink = graph.emplaceBlock<dashboardBlocks::imGUI_constellationSink<float>>();
    constellation_sink.id = "constellation_1";
    constellation_sink.title      = "checking_title_constellation";
    constellation_sink.panel_name = "Constellation";    
    constellation_sink.x_axis_label = "checking_x_axis_constellation";
    constellation_sink.y_axis_label = "checking_y_axis_constellation";
    constellation_sink.numberOfPoints = 256;

    /*
    ============================================================
    Widgets: all on the "Controls" panel. Checkbox, button, and dropdown
    all drive the same shared toggle between FREQ_LOW/FREQ_HIGH on the real source.
    Text box drives the complex source's offset live. Label reports the real frequency.
    ============================================================
    */
    auto& checkBox = graph.emplaceBlock<dashboardBlocks::dep_imGUI_checkBox<bool>>();
    checkBox.widget_id  = "freq_checkBox";
    checkBox.panel_name = "Controls";

    auto& button_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_button<bool>>();
    button_src.widget_id  = "freq_button";
    button_src.panel_name = "Controls";

    auto& checkBox_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();
    auto& button_drain   = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    auto& dropdownMenu = graph.emplaceBlock<dashboardBlocks::imGUI_dropDownMenu<float>>();
    dropdownMenu.widget_id     = "freq_dropdown";
    dropdownMenu.panel_name    = "Controls";
    dropdownMenu.target_property = "frequency";
    dropdownMenu.options       = std::vector<std::string>{"Low", "High"};
    auto& dropdown_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    auto& text_box = graph.emplaceBlock<dashboardBlocks::imGUI_textBox<float>>();
    text_box.widget_id  = "offset_input_1";
    text_box.panel_name = "Controls";
    auto& text_drain = graph.emplaceBlock<testing::NullSink<float>>();

    auto& freq_label = graph.emplaceBlock<dashboardBlocks::imGUI_textLabel<uint8_t>>();
    freq_label.widget_id  = "freq_label";
    freq_label.panel_name = "Controls";
    auto& freq_label_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

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

    checkBox.on_val_update = [&freq_toggle_state, apply_freq_toggle](bool new_state) {
        freq_toggle_state = new_state;
        apply_freq_toggle(freq_toggle_state);
    };
    checkBox.get_external_val = [&freq_toggle_state]() -> bool {
        return freq_toggle_state;
    };

    button_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](bool) {
        freq_toggle_state = !freq_toggle_state;
        apply_freq_toggle(freq_toggle_state);
    };

    dropdownMenu.on_val_update = [&freq_toggle_state, apply_freq_toggle](std::string selected_option) {
        freq_toggle_state = (selected_option == "High");
        apply_freq_toggle(freq_toggle_state);
    };
    
    dropdownMenu.get_external_val = [&freq_toggle_state]() -> std::string {
        return freq_toggle_state ? "High" : "Low";
    };

    // Text box drives the complex source's DC offset live, so typing a new
    // value re-centers the plotted constellation circle
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
    Connections
    ============================================================
    */
    // Real-valued fork: pacing path + three direct-stream sinks + tagged DataSet path
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }
    auto throttle_to_drain = graph.connect<"out", "in">(throttle, throttle_drain);
    if (!throttle_to_drain.has_value()) { return 1; }

    auto source_to_freq = graph.connect<"out", "in">(source, freq_sink);
    if (!source_to_freq.has_value()) { return 1; }

    auto source_to_waterfall = graph.connect<"out", "in">(source, waterfall_sink);
    if (!source_to_waterfall.has_value()) { return 1; }

    auto source_to_time = graph.connect<"out", "in">(source, time_sink);
    if (!source_to_time.has_value()) { return 1; }

    auto source_to_tagger = graph.connect<"out", "in">(source, tagger);
    if (!source_to_tagger.has_value()) { return 1; }
    auto tagger_to_s2ds = graph.connect<"out", "in">(tagger, s2ds);
    if (!tagger_to_s2ds.has_value()) { return 1; }
    auto s2ds_to_vector = graph.connect<"out", "in">(s2ds, vector_sink);
    if (!s2ds_to_vector.has_value()) { return 1; }

    // Complex-valued fork: pacing path + constellation sink
    auto sourceC_to_throttleC = graph.connect<"out", "in">(source_complex, throttle_complex);
    if (!sourceC_to_throttleC.has_value()) { return 1; }
    auto throttleC_to_drain = graph.connect<"out", "in">(throttle_complex, throttle_complex_drain);
    if (!throttleC_to_drain.has_value()) { return 1; }

    auto sourceC_to_constellation = graph.connect<"out", "in">(source_complex, constellation_sink);
    if (!sourceC_to_constellation.has_value()) { return 1; }

    // Widget uplinks -> NullSinks
    auto checkBox_to_drain = graph.connect<"out", "in">(checkBox, checkBox_drain);
    if (!checkBox_to_drain.has_value()) { return 1; }

    auto button_to_drain = graph.connect<"out", "in">(button_src, button_drain);
    if (!button_to_drain.has_value()) { return 1; }

    auto dropdown_to_drain = graph.connect<"out", "in">(dropdownMenu, dropdown_drain);
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

    scheduler.runAndWait();

    return 0;
}