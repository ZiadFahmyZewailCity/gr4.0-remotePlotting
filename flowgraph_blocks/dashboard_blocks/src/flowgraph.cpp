#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_button.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_checkBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_dropDownMenu.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textBox.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_textLabel.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_constellationSink.hpp>
#include <sstream>
#include <iomanip>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace dashboardBlocks = gr::dashboard_blocks;

int main() {
    gr::Graph graph;

    constexpr float FIXED_FREQ = 2343.75f;
    constexpr float OFFSET_POS = 50.0f;
    constexpr float OFFSET_NEG = -50.0f;

    // 1. Single generator - complex output traces a circle for the constellation sink
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<std::complex<float>>>();
    source.sample_rate = 48000.f;
    source.frequency   = FIXED_FREQ; 
    source.amplitude   = 25.0f;
    source.offset      = OFFSET_POS;
    source.phase       = 0.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Sin; 
    source.chunk_size  = 1024;

    // 2. Throttling the source 
    auto& throttle = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle.target_throughput = 48000.f; 
    throttle.busy_wait = false;           

    // 3. Downlink: constellation sink reads the stream directly
    auto& constellation_sink = graph.emplaceBlock<dashboardBlocks::imGUI_constellationSink<float>>();
    constellation_sink.title = "constellation_1";
    constellation_sink.numberOfPoints = 256;
    
    // Explicitly align bounds with the signal's swing
    constellation_sink.x_axis_min = 0;
    constellation_sink.x_axis_max = 100;
    constellation_sink.y_axis_min = 0;
    constellation_sink.y_axis_max = 100;

    // 4. Uplink Command Listener - CheckBox
    auto& checkBox_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_checkBox<bool>>();
    checkBox_src.widget_id = "freq_checkBox"; // ID preserved for JSON compatibility

    // 5. Uplink Command Listener - Button
    auto& button_src = graph.emplaceBlock<dashboardBlocks::dep_imGUI_button<bool>>();
    button_src.widget_id = "freq_button";

    // 6. Dummy drains 
    auto& checkBox_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();
    auto& button_drain   = graph.emplaceBlock<testing::NullSink<uint8_t>>();
    auto& throttle_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>(); 

    auto& dropdown_src = graph.emplaceBlock<dashboardBlocks::imGUI_dropDownMenu<uint8_t>>();
    dropdown_src.widget_id = "freq_dropdown";
    dropdown_src.target_property = "offset";
    dropdown_src.options = std::vector<std::string>{"Positive Offset", "Negative Offset"};

    //TextBox Input Listener 
    auto& text_box = graph.emplaceBlock<dashboardBlocks::imGUI_textBox<float>>();
    text_box.widget_id = "text_input_1";

    //Dummy drain for TextBox
    auto& text_drain = graph.emplaceBlock<testing::NullSink<float>>();

    // Text Label - displays whatever offset is currently active
    auto& offset_label = graph.emplaceBlock<dashboardBlocks::imGUI_textLabel<uint8_t>>();
    offset_label.widget_id = "freq_label"; // ID preserved for JSON compatibility

    //Dummy drain for the text label
    auto& offset_label_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    // Dummy drain for the dropdown
    auto& dropdown_drain = graph.emplaceBlock<testing::NullSink<uint8_t>>();

    // Shared state that both widgets drive
    bool offset_toggle_state = false; // false = POS, true = NEG

    // Applies the shared toggle state to the source's offset
    auto apply_offset_toggle = [&source](bool state) {
        gr::property_map old_props;
        old_props["offset"] = source.offset;

        float new_offset = state ? OFFSET_NEG : OFFSET_POS;
        gr::property_map new_props;
        new_props["offset"] = new_offset;

        source.offset = new_offset;
        source.settingsChanged(old_props, new_props);

        std::cout << "[Coordinator] Offset shifted to: " << new_offset << std::endl;
    };

    // CheckBox is real persisted state
    checkBox_src.on_val_update = [&offset_toggle_state, apply_offset_toggle](bool new_state) {
        offset_toggle_state = new_state;
        apply_offset_toggle(offset_toggle_state);
        std::cout << offset_toggle_state << "\n";
    };
    checkBox_src.get_external_val = [&offset_toggle_state]() -> bool {
        return offset_toggle_state;
    };

    // Button has no state of its own
    button_src.on_val_update = [&offset_toggle_state, apply_offset_toggle](bool) {
        offset_toggle_state = !offset_toggle_state;
        apply_offset_toggle(offset_toggle_state);
        std::cout << offset_toggle_state << "\n";
    };

    // Drop Down menu logic
    dropdown_src.on_val_update = [&offset_toggle_state, apply_offset_toggle](std::string selected_option) {
        offset_toggle_state = (selected_option == "Negative Offset");
        apply_offset_toggle(offset_toggle_state);
    };
    dropdown_src.get_external_val = [&offset_toggle_state]() -> std::string {
        return offset_toggle_state ? "Negative Offset" : "Positive Offset";
    };

    // TextBox Event Lambda - drives the source's frequency live, so typing a new
    // value speeds up or slows down the rotation.
    text_box.on_val_update = [&source](std::string msg) {
        try {
            float new_freq = std::stof(msg);

            gr::property_map old_props;
            old_props["frequency"] = source.frequency;

            gr::property_map new_props;
            new_props["frequency"] = new_freq;

            source.frequency = new_freq;
            source.settingsChanged(old_props, new_props);

            std::cout << "[Coordinator] Frequency shifted to: " << new_freq << " Hz" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[TextBox] Invalid frequency input: '" << msg << "' (" << e.what() << ")\n";
        }
    };

    // Text Label reports active offset
    offset_label.get_external_val = [&source]() -> std::string {
        std::ostringstream oss;
        oss << "Current Offset: " << std::fixed << std::setprecision(2) << source.offset;
        return oss.str();
    };

    /*
    ========================================================
    Connect Downlink (Parallel Architecture)
    ========================================================
    */
    auto source_to_constellation = graph.connect<"out", "in">(source, constellation_sink);
    if (!source_to_constellation.has_value()) { return 1; }

    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }

    auto throttle_to_drain = graph.connect<"out", "in">(throttle, throttle_drain);
    if (!throttle_to_drain.has_value()) { return 1; }

    /*
    Connect Uplink: UI Blocks -> NullSinks
    */
    auto checkBox_to_drain = graph.connect<"out", "in">(checkBox_src, checkBox_drain);
    if (!checkBox_to_drain.has_value()) { return 1; }

    auto button_to_drain = graph.connect<"out", "in">(button_src, button_drain);
    if (!button_to_drain.has_value()) { return 1; }

    auto dropdown_to_drain = graph.connect<"out", "in">(dropdown_src, dropdown_drain);
    if (!dropdown_to_drain.has_value()) { return 1; }

    auto text_to_drain = graph.connect<"out", "in">(text_box, text_drain);
    if (!text_to_drain.has_value()) { return 1; }

    auto offset_label_to_drain = graph.connect<"out", "in">(offset_label, offset_label_drain);
    if (!offset_label_to_drain.has_value()) { return 1; }

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