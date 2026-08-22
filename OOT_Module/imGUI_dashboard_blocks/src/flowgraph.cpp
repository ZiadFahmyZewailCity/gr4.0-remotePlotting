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
    constexpr float FREQ_CH2  = 1000.0f;   // fixed, no widget control
    constexpr float FREQ_CH3  = 3000.0f;   // fixed, no widget control

    /*
    ============================================================
    Real-valued chain: drives frequencySink, waterFallSink,
    timeSeries, and vectorSink (via tag + StreamToDataSet framing,
    which vectorSink actually requires - it takes DataSet<T>, not a plain stream)

    freq_sink now takes 3 data sources - the original widget-controlled sine wave
    plus two fixed-frequency sine waves purely to demonstrate multi-source plotting.
    Only "source" is ever touched by the checkbox/button/dropdown widgets below.
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

    // Second sine wave - channel 2 on the frequency plot, no widget attached
    auto& source2 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source2.sample_rate = 48000.f;
    source2.frequency   = FREQ_CH2;
    source2.amplitude   = 0.6f;
    source2.offset      = 0.0f;
    source2.phase       = 0.0f;
    source2.signal_type = basicBlocks::signal_generator::Type::Sin;
    source2.chunk_size  = 1024;

    auto& throttle2 = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle2.target_throughput = 48000.f;
    throttle2.busy_wait = false;
    auto& throttle2_drain = graph.emplaceBlock<testing::NullSink<float>>();

    // Third sine wave - channel 3 on the frequency plot, no widget attached
    auto& source3 = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source3.sample_rate = 48000.f;
    source3.frequency   = FREQ_CH3;
    source3.amplitude   = 0.3f;
    source3.offset      = 0.0f;
    source3.phase       = 0.0f;
    source3.signal_type = basicBlocks::signal_generator::Type::Sin;
    source3.chunk_size  = 1024;

    auto& throttle3 = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle3.target_throughput = 48000.f;
    throttle3.busy_wait = false;
    auto& throttle3_drain = graph.emplaceBlock<testing::NullSink<float>>();

    // data_sources set via direct assignment after construction, matching the working
    // pattern used for dropdown_src.options below - passing it through the constructor's
    // property_map instead throws ("settings could not be applied"), since pmtv stores
    // std::vector<std::string> as a generic Tensor<value> rather than the exact type
    // the reflected field expects.
    //
    // NOTE: this is a deliberate first test WITHOUT a settingsChanged override on the
    // sink yet - `in` (the port vector) will NOT get resized to match dataSources here,
    // so the graph.connect(..., "in#1")/"in#2" calls below are expected to fail. This is
    // intentional, to confirm the settings-application crash is fixed before adding the
    // resize logic next.
    auto& freq_sink = graph.emplaceBlock<dashboardBlocks::imGUI_frequencySink<float>>();
    // DIAGNOSTIC: temporarily reduced to 1 source to test whether freq_sink's 3-way
    // synchronous join (needing 1024 samples simultaneously across 3 independently-clocked
    // generators) is stalling `source`'s shared buffer via backpressure, which would starve
    // every other consumer of `source` (waterfall_sink/time_sink/vector_sink) even though
    // they have nothing to do with freq_sink directly. Revert to 3 sources once confirmed.
    freq_sink.dataSources = std::vector<std::string>{"channel_1"};
    // Direct assignment doesn't route through Settings::set(), so settingsChanged (which
    // resizes `in`/`FFTblocks` to match dataSources) never fires on its own - trigger it
    // manually. Our override only checks newSettings.contains("data_sources"), it never
    // reads the value, so a placeholder is fine here.
    freq_sink.settingsChanged({}, {{"data_sources", true}});
    freq_sink.id = "freq_sink";
    freq_sink.title = "Testing the title of the freq block";
    freq_sink.panel_name = "Frequency Domain";
    freq_sink.x_axis_label = "check_x_frequency";
    freq_sink.y_axis_label = "check_y_frequency";
    freq_sink.sampleRate = 48000.0f;
    freq_sink.windowSize = 1024;

    auto& time_sink = graph.emplaceBlock<dashboardBlocks::dep_imGUI_timeSeries<float>>();
    // Multi-source test: reusing the two already-existing generators (source = low/high
    // toggleable sine, source2 = fixed 1kHz sine) rather than adding new blocks. `in`
    // defaults to 1 element to match dataSources' own default, so this direct assignment
    // needs the same manual settingsChanged trigger as the other sinks to resize `in` to 2.
    time_sink.dataSources = std::vector<std::string>{"wave_toggle", "wave_1khz", "wave_3khz"};
    time_sink.settingsChanged({}, {{"data_sources", true}});
    time_sink.id   = "time_plot_1";
    time_sink.panel_name = "Time Domain";
    time_sink.title = "Testing the title of the time block";
    time_sink.x_axis_label = "Checking X axis works (#59)";
    time_sink.y_axis_label = "Checking y axis works (@23)";



    auto& waterfall_sink = graph.emplaceBlock<dashboardBlocks::imGUI_waterFallSink<float>>();
    // dataSources is left at its default ({"default_source_1"}), but `in` still defaults to
    // an EMPTY vector here (unlike dep_imGUI_timeSeries, which pre-sizes `in` to 1 at
    // declaration) - settingsChanged has to be triggered at least once or "in#0" won't exist
    // when connect() runs below.
    waterfall_sink.settingsChanged({}, {{"data_sources", true}});
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

    // Second and third tag+StreamToDataSet chains - vector_sink needs DataSet<T> per
    // source, so source2/source3 each get their own framing chain.
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
    // Now wired to 3 data sources for multi-source rendering.
    vector_sink.dataSources = std::vector<std::string>{"vec_toggle", "vec_1khz", "vec_3khz"};
    vector_sink.settingsChanged({}, {{"data_sources", true}});
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
    source_complex.amplitude   = 2.0f;
    source_complex.offset      = 50.0f;
    source_complex.phase       = 0.0f;
    source_complex.signal_type = basicBlocks::signal_generator::Type::Sin;
    source_complex.chunk_size  = 1024;

    auto& throttle_complex = graph.emplaceBlock<testing::SimCompute<std::complex<float>>>();
    throttle_complex.target_throughput = 48000.f;
    throttle_complex.busy_wait = false;
    auto& throttle_complex_drain = graph.emplaceBlock<testing::NullSink<std::complex<float>>>();

    // Second and third complex sine waves - extra data sources for constellation_sink
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

    auto& constellation_sink = graph.emplaceBlock<dashboardBlocks::imGUI_constellationSink<float>>();
    // Now wired to 3 data sources for multi-source rendering.
    constellation_sink.dataSources = std::vector<std::string>{"complex_1", "complex_2", "complex_3"};
    constellation_sink.settingsChanged({}, {{"data_sources", true}});
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

    // freq_sink's "in" is now a vector port (one per data source), so the compile-time
    // connect<"out","in"> syntax used elsewhere in this file can't target it - the port
    // count only exists at runtime. Confirmed runtime signature: connect(src, "out", dst, "in#N").
    auto source_to_freq0 = graph.connect(source, "out", freq_sink, "in#0");
    if (!source_to_freq0.has_value()) { return 1; }
    // DIAGNOSTIC: disabled for the isolation test above - source2/source3 still exist in
    // the graph but are now unconnected outputs, which GR4 should tolerate fine (produced
    // samples just get dropped, no consumer to deliver to).
    // auto source_to_freq1 = graph.connect(source2, "out", freq_sink, "in#1");
    // if (!source_to_freq1.has_value()) { return 1; }
    // auto source_to_freq2 = graph.connect(source3, "out", freq_sink, "in#2");
    // if (!source_to_freq2.has_value()) { return 1; }

    auto source2_to_throttle2 = graph.connect<"out", "in">(source2, throttle2);
    if (!source2_to_throttle2.has_value()) { return 1; }
    auto throttle2_to_drain = graph.connect<"out", "in">(throttle2, throttle2_drain);
    if (!throttle2_to_drain.has_value()) { return 1; }

    auto source3_to_throttle3 = graph.connect<"out", "in">(source3, throttle3);
    if (!source3_to_throttle3.has_value()) { return 1; }
    auto throttle3_to_drain = graph.connect<"out", "in">(throttle3, throttle3_drain);
    if (!throttle3_to_drain.has_value()) { return 1; }

    // waterfall_sink's "in" is also a vector port now (same reason as freq_sink above) -
    // only one data source is attached here, so it's index 0.
    auto source_to_waterfall = graph.connect(source, "out", waterfall_sink, "in#0");
    if (!source_to_waterfall.has_value()) { return 1; }

    // time_sink's "in" is now a vector port too (dep_imGUI_timeSeries got the same
    // multi-source treatment as the other sinks) - its default 1-element pre-sizing means
    // no settingsChanged trigger was needed above, but the compile-time connect<"out","in">
    // syntax still can't target a vector port, same as freq_sink/waterfall_sink/etc.
    // Now wired to 2 sources for the multi-source rendering test.
    auto source_to_time0 = graph.connect(source, "out", time_sink, "in#0");
    if (!source_to_time0.has_value()) { return 1; }
    auto source_to_time1 = graph.connect(source2, "out", time_sink, "in#1");
    if (!source_to_time1.has_value()) { return 1; }
    auto source_to_time2 = graph.connect(source3, "out", time_sink, "in#2");
    if (!source_to_time2.has_value()) { return 1; }

    auto source_to_tagger = graph.connect<"out", "in">(source, tagger);
    if (!source_to_tagger.has_value()) { return 1; }
    auto tagger_to_s2ds = graph.connect<"out", "in">(tagger, s2ds);
    if (!tagger_to_s2ds.has_value()) { return 1; }
    // vector_sink's "in" is a vector port - source1 goes to index 0.
    auto s2ds_to_vector = graph.connect(s2ds, "out", vector_sink, "in#0");
    if (!s2ds_to_vector.has_value()) { return 1; }

    auto source2_to_tagger2 = graph.connect<"out", "in">(source2, tagger2);
    if (!source2_to_tagger2.has_value()) { return 1; }
    auto tagger2_to_s2ds2 = graph.connect<"out", "in">(tagger2, s2ds2);
    if (!tagger2_to_s2ds2.has_value()) { return 1; }
    auto s2ds2_to_vector = graph.connect(s2ds2, "out", vector_sink, "in#1");
    if (!s2ds2_to_vector.has_value()) { return 1; }

    auto source3_to_tagger3 = graph.connect<"out", "in">(source3, tagger3);
    if (!source3_to_tagger3.has_value()) { return 1; }
    auto tagger3_to_s2ds3 = graph.connect<"out", "in">(tagger3, s2ds3);
    if (!tagger3_to_s2ds3.has_value()) { return 1; }
    auto s2ds3_to_vector = graph.connect(s2ds3, "out", vector_sink, "in#2");
    if (!s2ds3_to_vector.has_value()) { return 1; }

    // Complex-valued fork: pacing path + constellation sink
    auto sourceC_to_throttleC = graph.connect<"out", "in">(source_complex, throttle_complex);
    if (!sourceC_to_throttleC.has_value()) { return 1; }
    auto throttleC_to_drain = graph.connect<"out", "in">(throttle_complex, throttle_complex_drain);
    if (!throttleC_to_drain.has_value()) { return 1; }

    // constellation_sink's "in" is a vector port - source1 goes to index 0.
    auto sourceC_to_constellation = graph.connect(source_complex, "out", constellation_sink, "in#0");
    if (!sourceC_to_constellation.has_value()) { return 1; }

    auto sourceC2_to_throttleC2 = graph.connect<"out", "in">(source_complex2, throttle_complex2);
    if (!sourceC2_to_throttleC2.has_value()) { return 1; }
    auto throttleC2_to_drain = graph.connect<"out", "in">(throttle_complex2, throttle_complex2_drain);
    if (!throttleC2_to_drain.has_value()) { return 1; }
    auto sourceC2_to_constellation = graph.connect(source_complex2, "out", constellation_sink, "in#1");
    if (!sourceC2_to_constellation.has_value()) { return 1; }

    auto sourceC3_to_throttleC3 = graph.connect<"out", "in">(source_complex3, throttle_complex3);
    if (!sourceC3_to_throttleC3.has_value()) { return 1; }
    auto throttleC3_to_drain = graph.connect<"out", "in">(throttle_complex3, throttle_complex3_drain);
    if (!throttleC3_to_drain.has_value()) { return 1; }
    auto sourceC3_to_constellation = graph.connect(source_complex3, "out", constellation_sink, "in#2");
    if (!sourceC3_to_constellation.has_value()) { return 1; }

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

    //If the flowgraph will be killed using a SIGINT call, then you have to attach the schedluer to this lamda or define your own handler
    gr::dashboard_blocks::imGUI_DashboardRegistry::getInstance().set_stop_callback([&scheduler]() {
        std::ignore = scheduler.changeStateTo(gr::lifecycle::REQUESTED_STOP);
    });

    scheduler.runAndWait();

    return 0;
}