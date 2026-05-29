#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/basic/SignalGenerator.hpp> 
#include <gnuradio-4.0/testing/NullSources.hpp>
#include <gnuradio-4.0/POC_Blocks/dashBoardBridge.hpp>

namespace basicBlocks = gr::basic;
namespace testing = gr::testing;
namespace customBlocks = custom;

int main() {
    gr::Graph graph;

    // --- 1. SIGNAL GENERATOR BLOCK ---
    auto& source = graph.emplaceBlock<basicBlocks::SignalGenerator<float>>();
    source.sample_rate = 48000.f;
    source.frequency   = 1.0f; 
    source.amplitude   = 1.0f;
    source.signal_type = basicBlocks::signal_generator::Type::Cos;
    source.chunk_size  = 1024;

    // --- 2. SIM COMPUTE BLOCK (Throttle) ---
    auto& throttle = graph.emplaceBlock<testing::SimCompute<float>>();
    throttle.target_throughput = 48000.f; 
    throttle.busy_wait = false;           

    // --- 3. DASHBOARD BRIDGE BLOCK ---
    auto& bridge = graph.emplaceBlock<customBlocks::DashboardBridge<float>>();
    bridge.multiplier = 1.0f; 
    
    // NEW: Pass the config file layout to the block
    bridge.config_file = "../POC_Blocks/assets/config.json"; 

    // Connections
    auto source_to_throttle = graph.connect<"out", "in">(source, throttle);
    if (!source_to_throttle.has_value()) return 1; 

    auto throttle_to_bridge = graph.connect<"out", "in">(throttle, bridge);
    if (!throttle_to_bridge.has_value()) return 1; 

    // Run
    gr::scheduler::Simple<> scheduler;
    if (!scheduler.exchange(std::move(graph)).has_value()) return 1;
    scheduler.runAndWait();

    return 0;
}