#ifndef GNURADIO_DASHBOARDBRIDGE_HPP
#define GNURADIO_DASHBOARDBRIDGE_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>

#include <thread>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace custom {

typedef websocketpp::server<websocketpp::config::asio> server;

GR_REGISTER_BLOCK("custom::DashboardBridge", custom::DashboardBridge, [float])

template<typename T>
struct DashboardBridge : gr::Block<DashboardBridge<T>> {
    using Description = gr::Doc<R""(
        Hosts a WebSocket server on port 9000.
        Pushes a JSON config file on connection, streams live DSP data via binary frames,
        and receives text commands to update the internal multiplier.
    )"">;

    // --- PORTS & PARAMETERS ---
    gr::PortIn<T> in;
    
    gr::Annotated<T, "multiplier", gr::Visible> multiplier = static_cast<T>(1.0);
    
    // NEW: Parameter to hold the path to the config file
    gr::Annotated<std::string, "config_file", gr::Visible> config_file = "";

    GR_MAKE_REFLECTABLE(DashboardBridge, in, multiplier, config_file);

    // --- INFRASTRUCTURE ---
    server m_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_connections;
    std::thread m_server_thread;
    std::string m_cached_config; // Caches the JSON string so we don't read the disk repeatedly

    // --- LIFECYCLE HOOKS ---
    void start() {
        try {
            // 1. Read the Config File into memory
            if (!config_file.value.empty()) {
                std::ifstream t(config_file.value);
                if (t.good()) {
                    std::stringstream buffer;
                    buffer << t.rdbuf();
                    m_cached_config = buffer.str();
                    std::cout << "[DashboardBridge] Loaded config file: " << config_file.value << std::endl;
                } else {
                    std::cerr << "[DashboardBridge] WARNING: Could not open config file: " << config_file.value << std::endl;
                }
            }

            m_server.clear_access_channels(websocketpp::log::alevel::all);
            m_server.init_asio();

            // 2. Send the config immediately when a client connects
            m_server.set_open_handler([this](websocketpp::connection_hdl hdl) {
                m_connections.insert(hdl);
                std::cout << "[DashboardBridge] Frontend Connected!" << std::endl;
                
                if (!m_cached_config.empty()) {
                    // Send the cached JSON as a TEXT frame
                    m_server.send(hdl, m_cached_config, websocketpp::frame::opcode::text);
                    std::cout << "[DashboardBridge] Config pushed to frontend." << std::endl;
                }
            });

            m_server.set_close_handler([this](websocketpp::connection_hdl hdl) {
                m_connections.erase(hdl);
                std::cout << "[DashboardBridge] Frontend Disconnected." << std::endl;
            });

            // 3. Receive slider updates
            m_server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg) {
                if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                    std::string payload = msg->get_payload();
                    
                    // Note: For this MVP, we still assume the raw string is just "3.5"
                    // If you send full JSON from Emscripten, you will need nlohmann::json::parse() here!
                    try {
                        float new_val = std::stof(payload);
                        this->multiplier = static_cast<T>(new_val);
                    } catch (...) {
                        std::cerr << "[DashboardBridge] Failed to parse command: " << payload << std::endl;
                    }
                }
            });

            m_server.listen(9000);
            m_server.start_accept();

            m_server_thread = std::thread([this]() {
                m_server.run();
            });

        } catch (websocketpp::exception const & e) {
            std::cerr << "[DashboardBridge] WebSocket failed: " << e.what() << std::endl;
            throw std::runtime_error("WebSocket Server Failed to Start");
        }
    }

    void stop() {
        m_server.stop_listening();
        for (auto& hdl : m_connections) {
            m_server.close(hdl, websocketpp::close::status::normal, "Stopping");
        }
        if (m_server_thread.joinable()) m_server_thread.join();
    }

    // --- REAL-TIME DSP LOOP ---
    [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
        const std::size_t nSamples = input.size();
        if (nSamples == 0) return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;

        const T* raw_data = input.data();
        std::vector<T> processed_data(nSamples);
        
        for(std::size_t i = 0; i < nSamples; i++) {
            processed_data[i] = raw_data[i] * multiplier.value;
        }

        if (!m_connections.empty()) {
            std::size_t num_bytes = nSamples * sizeof(T);
            for (auto hdl : m_connections) {
                m_server.send(hdl, processed_data.data(), num_bytes, websocketpp::frame::opcode::binary);
            }
        }

        std::ignore = input.consume(nSamples);
        return gr::work::Status::OK;
    }
};
} // namespace custom
#endif