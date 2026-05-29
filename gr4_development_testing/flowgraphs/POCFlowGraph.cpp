/*
This outline is for running a dynamic flowgraph pushing to a Web UI.
It acts as the Coordinator, connecting the Source, Throttle, and Bridge,
and routing UI commands back to the Source.
*/

#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>

// Include your custom OOT block
// Note: Adjust this include path based on exactly where you saved it!
#include <gnuradio-4.0/POC_Blocks/dashBoardBridge.hpp> 

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace pocBlocks = gr::POC_Blocks;

int main() {
    gr::Graph graph;

    /*
    We need 3 blocks for this live test:
    - Source: Generates the base wave
    - Throttle: Keeps the CPU from crashing the browser by limiting throughput
    - Bridge: Streams data and hosts the WebSocket server
    */

    // --- 1. SIGNAL GENERATOR BLOCK (Source) ---
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = 50.0f; 
    source.amplitude   = 1.0f;
    source.offset      = 0.0f;
    source.phase       = 0.0f;
    //Signal type
    source.signal_type = basicBlocks::signal_generator::Type::Sin; 
    source.chunk_size  = 1024;

    // --- 2. SIM COMPUTE BLOCK (The Throttle) ---
    // Without this, the flowgraph runs at CPU speed and crashes the WebSocket
    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f; // Throttle to exactly 48kS/s
    throttle.busy_wait = false;           // Use sleep() instead of burning CPU cycles

    // --- 3. DASHBOARD BRIDGE BLOCK (Pure Sink & Remote Control) ---
    auto& bridge = graph.emplaceBlock<pocBlocks::dashBoardBridge<float>>();
    
    // Set the path to your UI config file. 
    // IMPORTANT: Make sure this path is correct relative to where you run the executable!
    bridge.config_file = "../POC_Blocks/assets/config.json"; 


    // --- THE COORDINATOR LINK ---
    // This is the magic. When the bridge receives a frequency command from the UI,
    // it pulls its internal trigger. This lambda intercepts it and updates the source block.
    bridge.on_freq_update = [&source](float new_freq) {
        
        // 1. Save the old frequency state for the hook
        gr::property_map old_props;
        old_props["frequency"] = source.frequency;
        
        // 2. Prepare the new frequency state
        gr::property_map new_props;
        new_props["frequency"] = new_freq;

        // 3. Update the raw C++ variable
        source.frequency = new_freq;

        // 4. THE FIX: Force the block to recalculate its internal DSP math!
        source.settingsChanged(old_props, new_props);

        std::cout << "[Coordinator] Frequency shifted to: " << new_freq << " Hz" << std::endl;
    };


    /*
    Connect them: Source -> Throttle -> Bridge
    */
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }

    auto throttle_to_bridge = graph.connect<"out", "dataStream">(throttle, bridge);    
    if (!throttle_to_bridge.has_value()) { return 1; }


    /*
    Pass the graph to the scheduler and run indefinitely.
    */
    std::cout << "Starting GNU Radio 4.0 Flowgraph Coordinator..." << std::endl;
    gr::scheduler::Simple<> scheduler;

    if (!scheduler.exchange(std::move(graph)).has_value()) {
        std::cerr << "Flowgraph failed to start!" << std::endl;
        return 1;
    }

    scheduler.runAndWait();

    return 0;
}