#ifndef GNURADIO_DASHBOARDBRIDGE_HPP
#define GNURADIO_DASHBOARDBRIDGE_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/roles/server_endpoint.hpp>
#include <websocketpp/server.hpp> // Added so your server typedef works


#include <fstream>
#include <sstream>
#include <functional>
#include <nlohmann/json.hpp> 

/*
    asynchronous library for websockets
*/

namespace gr::POC_Blocks {

    typedef websocketpp::server<websocketpp::config::asio> server;

    //Inherit from block and use template
    template<typename T>
    struct dashBoardBridge : gr::Block<dashBoardBridge<T>> {

        //Description about block
        using Description = gr::Doc<R""(This is a block made to test communication between a GR4 flowgraph and 
        a imGUI dashboard thats meant to be configurable.
        The blocks hosts a websocket server on a port
        Pushes a JSON config file on connection to the dashboard
        begins streaming the live data via binary frames
        receives test commands to update an internal variable
        Currently only takes in 1 input)"">;

        //Ports
        //Input ports and no output port
        gr::PortIn<T> dataStream;
        
        //This is a variable which is exposed to the outside 
        //Will be used to adjust the frequency of the sin wave
        gr::Annotated<T, "sinFrequency", gr::Visible> frequencyUpdate = static_cast<T>(1.0);

        //The adderess to the config file is passed here in this variable
        gr::Annotated<std::string, "config_file" ,gr::Visible> config_file = "";

        //Default constructor
        dashBoardBridge() : gr::Block<dashBoardBridge<T>>("dashBoardBridge") {}

        //Make sure we can track these variables
        GR_MAKE_REFLECTABLE(dashBoardBridge, dataStream, frequencyUpdate, config_file);

        // STYLE TIP: Use std::function for callbacks to other blocks. 
        // This acts as a trigger we will pull when the UI slider moves.
        std::function<void(float)> on_freq_update = nullptr;

        //Intializing server on another thread
        server flowGraphServer;
        std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> flowGraphConnections;
        std::thread flowGraphThread;
        
        //Defining start state
        void start() {

            // Attempt to read the config file 
            try {
                //Start the aysnchronous communication
                flowGraphServer.init_asio();

                // Call back upon connection
                // Send the config on connection with dashboard
                flowGraphServer.set_open_handler([this](websocketpp::connection_hdl hdl) {
                    flowGraphConnections.insert(hdl);
                    std::cout << "Frontend Connected" << std::endl;

                    //Attempt to read the file
                    if (!config_file.value.empty()) {
                        std::ifstream t(config_file.value);
                        if (t.good()) {
                            std::stringstream buffer;
                            buffer << t.rdbuf(); 
                            std::string temporary_config = buffer.str();
                            
                            // Send it out over the text channel immediately
                            flowGraphServer.send(hdl, temporary_config, websocketpp::frame::opcode::text);
                            std::cout << "Loaded and sent config file: " << config_file.value << std::endl;
                        } else {
                            std::cerr << "Could not open config file: " << config_file.value << std::endl;
                        }
                    }
                });

                // Call back upon closing connection 
                flowGraphServer.set_close_handler([this](websocketpp::connection_hdl hdl) {
                    flowGraphConnections.erase(hdl);
                    std::cout << "Frontend Disconnected." << std::endl;
                });


                // Call back for recieving messages
                flowGraphServer.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg) {
                    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                        try {
                            auto json_msg = nlohmann::json::parse(msg->get_payload());
                            if (json_msg["target"] == "slider_freq" && this->on_freq_update) {
                                this->on_freq_update(json_msg["value"]);
                            }
                        } catch (...) {
                            std::cerr << "Failed to parse command from dashboard" << std::endl;
                        }
                    }
                });

                // Tell the server to open port 9000 and start listening
                flowGraphServer.listen(9000);
                flowGraphServer.start_accept();

                // run the server in a background thread
                flowGraphThread = std::thread([this]() { flowGraphServer.run(); });

            } 
            catch (const std::exception& e) {
                std::cerr << "[DashboardBridge] Error during start: " << e.what() << std::endl;
            }
        }

        //Defining stop state
        void stop() {
            flowGraphServer.stop_listening();
            for (auto& hdl : flowGraphConnections) {
                flowGraphServer.close(hdl, websocketpp::close::status::normal, "Stopping flowgraph Server");
            }
            if (flowGraphThread.joinable()) {
                flowGraphThread.join();
            }
        }

        //Use processBuilk here as we take packages of data and send them at once
        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
            const std::size_t nSamples = input.size();
            
            //We check to see if we have recieved samples, if not we set the status to insufficient
            if (nSamples == 0) 
            {
                return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
            }

            //We take some inputs data
            //Have a pointer point at it and pass it to be sent 
            if (!flowGraphConnections.empty()) {
                std::size_t num_bytes = nSamples * sizeof(T);
                const void* raw_data_ptr = static_cast<const void*>(input.data());
                
                for (auto hdl : flowGraphConnections) {
                    flowGraphServer.send(hdl, raw_data_ptr, num_bytes, websocketpp::frame::opcode::binary);
                }
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }

    };

} // namespace gr::POC_Blocks

//Register block in block registry
GR_REGISTER_BLOCK("gr::POC_Blocks::dashBoardBridge", gr::POC_Blocks::dashBoardBridge, [float])

#endif