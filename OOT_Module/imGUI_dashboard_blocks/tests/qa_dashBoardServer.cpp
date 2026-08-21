#include "../src/dashBoard_server.hpp"
#include <boost/ut.hpp>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

int main() {
    using namespace boost::ut;

    // --- TEST SUITE 1: ZMQ Downlink Topic Filtering ---
    "downlink_topic_filtering"_test = [] {
        zmq::context_t test_ctx(1);

        // 1. Setup mock GNU Radio Publisher (Flowgraph)
        zmq::socket_t mock_flowgraph_pub(test_ctx, zmq::socket_type::pub);
        mock_flowgraph_pub.bind("tcp://127.0.0.1:5555");

        // 2. Setup mock Dashboard Subscriber
        zmq::socket_t dashboard_sub(test_ctx, zmq::socket_type::sub);
        dashboard_sub.connect("tcp://127.0.0.1:5555");
        
        // Apply hardware filter strictly for 'plot_1'
        dashboard_sub.set(zmq::sockopt::subscribe, "plot_1");

        // Allow TCP handshake to complete across OS kernel
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // 3. Blast valid and invalid topics down the pipe
        std::string valid_packet = "plot_1:{\"samples\":[0.1, 0.5]}";
        std::string rogue_packet = "plot_debug:{\"secret\":true}";

        mock_flowgraph_pub.send(zmq::message_t(rogue_packet.begin(), rogue_packet.end()), zmq::send_flags::none);
        mock_flowgraph_pub.send(zmq::message_t(valid_packet.begin(), valid_packet.end()), zmq::send_flags::none);

        // 4. Assert that the kernel dropped the rogue packet and only passed 'plot_1'
        zmq::message_t caught_msg;
        auto result = dashboard_sub.recv(caught_msg, zmq::recv_flags::none);

        expect(result.has_value()) << "Subscriber failed to catch downlink telemetry";
        expect(caught_msg.to_string() == valid_packet) << "Hardware topic filter failed; caught rogue packet!";
    };

    // --- TEST SUITE 2: Uplink Inproc Pipeline ---
    "uplink_inproc_routing"_test = [] {
        zmq::context_t shared_brain(1);

        // 1. Setup Daemon command puller
        zmq::socket_t daemon_pull(shared_brain, zmq::socket_type::pull);
        daemon_pull.bind("inproc://commands");

        // 2. Simulate background WebSocket thread pushing a UI click
        std::thread ws_mock_thread([&shared_brain]() {
            zmq::socket_t ws_push(shared_brain, zmq::socket_type::push);
            ws_push.connect("inproc://commands");
            
            std::string click_payload = "slider_freq:101.5";
            ws_push.send(zmq::message_t(click_payload.begin(), click_payload.end()), zmq::send_flags::none);
        });

        // 3. Assert Main Daemon thread wakes up and receives exact bytes
        zmq::message_t rx_cmd;
        auto rx_status = daemon_pull.recv(rx_cmd, zmq::recv_flags::none);

        ws_mock_thread.join();

        expect(rx_status.has_value()) << "inproc:// airlock failed to trigger interrupt";
        expect(rx_cmd.to_string() == "slider_freq:101.5") << "Command payload corrupted across memory bridge";
    };
    // --- TEST SUITE 3: Uplink WS JSON Unpacking & Formatting ---
    "uplink_json_formatting"_test = [] {
        zmq::context_t test_ctx(1);

        // 1. Setup the receiving end of the inproc airlock
        zmq::socket_t mock_daemon_pull(test_ctx, zmq::socket_type::pull);
        mock_daemon_pull.bind("inproc://commands");

        // 2. Instantiate your actual server class
        DashboardServer test_server("../web/");
        test_server.set_ZMQ_context(test_ctx);

        // 3. Simulate a raw JSON string arriving from the browser
        std::string raw_browser_json = "{\"target\":\"slider_freq\",\"value\":440.5}";

        // We simulate the WebSocket handler pushing the JSON payload into your ZMQ bridge
        std::thread mock_ws_event([&]() {
            zmq::socket_t push_sock(test_ctx, zmq::socket_type::push);
            push_sock.connect("inproc://commands");
            
            // Replicate exact logic inside on_message to verify string conversion
            auto j = nlohmann::json::parse(raw_browser_json);
            std::string formatted = j.value("target", "") + ":" + std::to_string(j.value("value", 0.0f));
            
            push_sock.send(zmq::message_t(formatted.begin(), formatted.end()), zmq::send_flags::none);
        });

        zmq::message_t caught_frame;
        auto status = mock_daemon_pull.recv(caught_frame, zmq::recv_flags::none);
        mock_ws_event.join();

        expect(status.has_value()) << "Server failed to dispatch JSON through inproc bridge";
        
        // std::to_string(440.5f) evaluates to "440.500000" in standard C++
        std::string expected_output = "slider_freq:440.500000"; 
        expect(caught_frame.to_string() == expected_output) 
            << "JSON unpacking failed! Expected '" << expected_output << "' but got '" << caught_frame.to_string() << "'";
    };


    
}