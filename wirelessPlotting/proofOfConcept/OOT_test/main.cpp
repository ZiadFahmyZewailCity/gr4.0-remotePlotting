// Graph
#include <gnuradio-4.0/Graph.hpp>

// --- GR4 Basic Blocks ---
#include <gnuradio-4.0/basic/SignalGenerator.hpp>

//Scheduler
#include <gnuradio-4.0/Scheduler.hpp>

//Custom block for pushing data to ZMQ socket
#include "ZMQBlock.hpp"

using complexFloat_sigSource = gr::basic::SignalGenerator<float>;
using complexFloat_ZMQ = custom::ZMQBlock<float>;

int main() {

    //Flow graph object, my understanding this is where all the blocks should exist to be connected
    gr::Graph figure;

    //SIGNAL SOURCE BLOCK
    //Creating a signal source that outputs a complex float
    auto& sig_gen = figure.emplaceBlock<complexFloat_sigSource>();

    sig_gen.sample_rate = 32e3;
    sig_gen.frequency = 1e3;
    sig_gen.signal_type = gr::basic::signal_generator::Type::FastSin;
    sig_gen.amplitude = 1.0;


    //ZMQ block to 
    auto& ZMQ_ComInstance = figure.emplaceBlock<complexFloat_ZMQ>();
    ZMQ_ComInstance.address = "tcp://127.0.0.1:5555";

    //Wouldnt let me run the code unless i did a check on the output
    auto conn_result = figure.connect<"out", "in">(sig_gen, ZMQ_ComInstance);
    if (!conn_result.has_value()) {
        return 1; 
    }

    // 1. Instantiate the Simple scheduler
    gr::scheduler::Simple<> sched;

    //Need another check to run 
    if (!sched.exchange(std::move(figure)).has_value()) {
        std::cerr << "Error: Failed to attach graph to scheduler." << std::endl;
        return 1; 
    }
    
    //Need another check to run
    if (!sched.runAndWait().has_value()) {
    std::cerr << "Error: Failed to attach graph to scheduler." << std::endl;
    return 1; 
    }

    return 0;
}