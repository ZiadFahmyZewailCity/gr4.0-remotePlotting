/*
This outline is for running a dynamic flowgraph pushing to a Web UI
*/

#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>

// Include your custom OOT block
// Note: Adjust this include path based on exactly where you saved it in your POC_Blocks directory!
#include <gnuradio-4.0/POC_Blocks/DashboardBridge.hpp> 

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace customBlocks = custom;

int main() {
    gr::Graph graph;

    /*
    We need 3 blocks for this live test:
    - Source: Generates the base wave
    - Throttle: Keeps the CPU from crashing the browser by limiting throughput
    - Bridge: Applies the math multiplier and hosts the WebSocket server
    */

    // --- 1. SIGNAL GENERATOR BLOCK (Source) ---
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = 1.0f; // 1 Hz wave, matching your math.sin(t) test
    source.amplitude   = 1.0f;
    source.offset      = 0.0f;
    source.phase       = 0.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Cos;
    source.chunk_size  = 1024;

    // --- 2. SIM COMPUTE BLOCK (The Throttle) ---
    // Without this, the flowgraph runs at CPU speed and crashes the WebSocket
    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f; // Throttle to exactly 48kS/s
    throttle.busy_wait = false;           // Use sleep() instead of burning CPU cycles

    // --- 3. DASHBOARD BRIDGE BLOCK (Processing & Sink) ---
    auto& bridge = graph.emplaceBlock<customBlocks::DashboardBridge<float>>();
    bridge.multiplier = 1.0f; // Starting value


    /*
    Connect them: Source -> Throttle -> Bridge
    */
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) { return 1; }

    auto throttle_to_bridge = graph.connect<"out", "in">(throttle, bridge);
    if (!throttle_to_bridge.has_value()) { return 1; }


    /*
    Pass the graph to the scheduler and run indefinitely.
    You will stop this via Ctrl+C in the terminal.
    */
    gr::scheduler::Simple<> scheduler;

    if (!scheduler.exchange(std::move(graph)).has_value()) {
        return 1;
    }

    scheduler.runAndWait();

    return 0;
}