#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp>
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_frequencySink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_waterFallSink.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_checkBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_button.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_dropDownMenu.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textLabel.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_slider.hpp> 
#include <sstream>
#include <iomanip>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace dashboardBlocks = gr::dashboard_blocks;

int main() {
    gr::Graph graph;

    constexpr float FREQ_LOW  = 2343.75f;
    constexpr float FREQ_HIGH = 4687.5f;
    constexpr float FREQ_CH2  = 1000.0f;   

    /*
    ============================================================
    Real-valued chain: timeSeries, waterFallSink, and frequencySink
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

    auto& time_sink = graph.emplaceBlock<dashboardBlocks::imGUI_timeSeries<float>>();
    time_sink.dataSources = std::vector<std::string>{"wave_toggle", "wave_1khz"};
    time_sink.settingsChanged({}, {{"data_sources", true}});
    time_sink.id   = "time_plot_1";
    time_sink.panel_name = "Time Domain";
    time_sink.title = "Real Time Series (Float)";
    time_sink.x_axis_label = "Samples";
    time_sink.y_axis_label = "Amplitude";

    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    waterfall_sink.settingsChanged({}, {{"data_sources", true}});
    waterfall_sink.id = "waterfall_id";
    waterfall_sink.title        = "check_waterfall_title";
    waterfall_sink.x_axis_label = "check_x-axis label";
    waterfall_sink.y_axis_label = "check_y-axis label";
    waterfall_sink.panel_name   = "Frequency Domain";
    waterfall_sink.sampleRate   = 48000.0f;
    waterfall_sink.windowSize   = 1024;
    waterfall_sink.history_size = 100;

    auto& freq_sink = graph.emplaceBlock<dashboardBlocks::imGUI_frequencySink<float>>();
    freq_sink.dataSources = std::vector<std::string>{"freq_1", "freq_2"};
    freq_sink.settingsChanged({}, {{"data_sources", true}});
    freq_sink.id = "freq_sink_id";
    freq_sink.title = "Real Frequency Spectrum";
    freq_sink.x_axis_label = "Frequency (Hz)";
    freq_sink.y_axis_label = "Magnitude";
    freq_sink.panel_name = "Frequency Domain";
    freq_sink.sampleRate = 48000.0f;
    freq_sink.windowSize = 1024;

    /*
    ============================================================
    Widgets
    ============================================================
    */
    auto& freq_slider = graph.emplaceBlock<dashboardBlocks::imGUI_slider<float>>();
    freq_slider.widget_id = "freq_slider";
    freq_slider.panel_name = "Controls";
    freq_slider.title = "Source Frequency (Hz)";
    freq_slider.min_val = 0.0f;
    freq_slider.max_val = 24000.0f; 
    freq_slider.current_val = source.frequency;
    auto& freq_slider_drain = graph.emplaceBlock<testing::NullSink<float>>();

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

    auto apply_freq_update = [&source](float new_freq) {
        gr::property_map old_props;
        old_props["frequency"] = source.frequency;

        gr::property_map new_props;
        new_props["frequency"] = new_freq;

        source.frequency = new_freq;
        source.settingsChanged(old_props, new_props);

        std::cout << "[Coordinator] Frequency shifted to: " << new_freq << " Hz" << std::endl;
    };

    auto apply_freq_toggle = [&freq_toggle_state, apply_freq_update](bool state) {
        float new_freq = state ? FREQ_HIGH : FREQ_LOW;
        apply_freq_update(new_freq);
    };

    freq_slider.on_val_update = [apply_freq_update](float val) {
        apply_freq_update(val);
    };
    freq_slider.get_external_val = [&source]() -> float {
        return source.frequency;
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

    text_box.on_val_update = [&source](std::string msg) {
        try {
            float new_offset = std::stof(msg);

            gr::property_map old_props;
            old_props["offset"] = source.offset;

            gr::property_map new_props;
            new_props["offset"] = new_offset;

            source.offset = new_offset;
            source.settingsChanged(old_props, new_props);

            std::cout << "[Coordinator] Main wave offset shifted to: " << new_offset << std::endl;
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
    auto source_to_time0 = graph.connect(source, "out", time_sink, "in#0");
    if (!source_to_time0.has_value()) { return 1; }
    auto source_to_time1 = graph.connect(source2, "out", time_sink, "in#1");
    if (!source_to_time1.has_value()) { return 1; }

    auto source_to_waterfall = graph.connect(source, "out", waterfall_sink, "in#0");
    if (!source_to_waterfall.has_value()) { return 1; }

    auto source_to_freq0 = graph.connect(source, "out", freq_sink, "in#0");
    if (!source_to_freq0.has_value()) { return 1; }
    auto source_to_freq1 = graph.connect(source2, "out", freq_sink, "in#1");
    if (!source_to_freq1.has_value()) { return 1; }

    // Drain Connections for UI Elements
    auto slider_to_drain = graph.connect<"out", "in">(freq_slider, freq_slider_drain);
    if (!slider_to_drain.has_value()) { return 1; }
    
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