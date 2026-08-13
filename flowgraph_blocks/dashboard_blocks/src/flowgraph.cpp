#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/dep_imGUI_timeSeries.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_button.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_checkBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_waterFallSink.hpp> 
#include <gnuradio-4.0/dashboard_blocks/imGUI_dropDownMenu.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textLabel.hpp>
#include <sstream>
#include <iomanip>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace dashboardBlocks = gr::dashboard_blocks;

int main() {
    gr::Graph graph;

    constexpr float FREQ_LOW  = 2343.75f;
    constexpr float FREQ_HIGH = 4687.5f;

    // 1. Single generator
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = FREQ_LOW; 
    source.amplitude   = 1.0f;
    source.offset      = 0.0f;
    source.phase       = 0.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Sin; 
    source.chunk_size  = 1024;

    // 2. Throttling the source 
    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f; 
    throttle.busy_wait = false;           

    // 3. Downlink Waterfall Sink (Assigned to Frequency Domain panel)
    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    waterfall_sink.title = "waterfall_plot_1";
    waterfall_sink.panel_name = "Frequency Domain";
    waterfall_sink.sampleRate = 48000.0f; 
    waterfall_sink.windowSize = 1024;
    waterfall_sink.history_size = 100;

    // 3b. Downlink Time Series Sink (Assigned to Time Domain panel)
    auto& time_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    time_sink.topic_id = "time_plot_1";
    time_sink.panel_name = "Time Domain";

    // 4. Uplink Command Listener - CheckBox
    auto& checkBox_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_checkBox<bool>>();
    checkBox_src.widget_id = "freq_checkBox";

    // 5. Uplink Command Listener - Button
    auto& button_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_button<bool>>();
    button_src.widget_id = "freq_button";

    // 6. Dummy drains
    auto& checkBox_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();
    auto& button_drain   = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    auto& dropdown_src = graph.emplaceBlock<dashboardBlocks::imGUI_dropDownMenu<uint8_t>>();
    dropdown_src.widget_id = "freq_dropdown";
    dropdown_src.target_property = "frequency";
    dropdown_src.options = std::vector<std::string>{"Low", "High"};

    // TextBox Input Listener 
    auto& text_box = graph.emplaceBlock<dashboardBlocks::imGUI_textBox<float>>();
    text_box.widget_id = "text_input_1";

    // Dummy drain for TextBox
    auto& text_drain = graph.emplaceBlock<testing::NullSink<float>>();

    // Text Label - displays whatever frequency is currently active
    auto& freq_label = graph.emplaceBlock<dashboardBlocks::imGUI_textLabel<uint8_t>>();
    freq_label.widget_id = "freq_label";

    // Dummy drain for the text label
    auto& freq_label_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    // Dummy drain for the dropdown
    auto& dropdown_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    // Shared state that both widgets drive
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
        std::cout << freq_toggle_state << "\n";
    };
    checkBox_src.get_external_val = [&freq_toggle_state]() -> bool {
        return freq_toggle_state;
    };

    button_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](bool) {
        freq_toggle_state = !freq_toggle_state;
        apply_freq_toggle(freq_toggle_state);
        std::cout << freq_toggle_state << "\n";
    };

    dropdown_src.on_val_update = [&freq_toggle_state, apply_freq_toggle](std::string selected_option) {
        freq_toggle_state = (selected_option == "High");
        apply_freq_toggle(freq_toggle_state);
    };
    dropdown_src.get_external_val = [&freq_toggle_state]() -> std::string {
        return freq_toggle_state ? "High" : "Low";
    };

    text_box.on_val_update = [](std::string msg) {
        std::cout << "\n========================================\n";
        std::cout << "[TextBox] User Input Received: " << msg << "\n";
        std::cout << "========================================\n" << std::endl;
    };

    freq_label.get_external_val = [&source]() -> std::string {
        std::ostringstream oss;
        oss << "Current Frequency: " << std::fixed << std::setprecision(2) << source.frequency << " Hz";
        return oss.str();
    };

    /*
    Connect Downlinks: Source -> Throttle -> Sinks
    */
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }

    auto throttle_to_waterfall = graph.connect<"out", "in">(throttle, waterfall_sink);
    if (!throttle_to_waterfall.has_value()) { return 1; }

    auto throttle_to_time = graph.connect<"out", "in">(throttle, time_sink);
    if (!throttle_to_time.has_value()) { return 1; }

    /*
    Connect Uplinks: UI Blocks -> NullSink
    */
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

    scheduler.runAndWait();

    return 0;
}