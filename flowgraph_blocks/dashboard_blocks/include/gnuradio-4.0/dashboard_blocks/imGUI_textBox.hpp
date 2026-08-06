#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_TEXTBOX_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_TEXTBOX_HPP

#include <cstring>
#include <exception>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <iostream>
#include <zmq.hpp>
#include <string>
#include <functional>


namespace gr::dashboard_blocks {

    template <typename T>
    struct imGUI_textBox : gr::Block<imGUI_textBox<T>> {

        //TO DO: Add Proper Description
        using Description = gr::Doc<R""()"">;

        //Widget ID (Must be given a unique id by user)
        gr::Annotated<std::string, "widget_id", gr::Visible> widget_id = "imGUI_textBox_default";
        
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5556";
        
        gr::PortOut<T> out;

        //Lamdas for updating variable being controlled
        //Lamda for updating value
        std::function<void(std::string)> on_val_update = nullptr;

        //ZMQ related variables
        zmq::context_t zmq_ctx{1};
        zmq::socket_t subscriber;

        public:

        imGUI_textBox(gr::property_map initial_settings = {})
            : gr::Block<imGUI_textBox<T>>(initial_settings)
        {
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", ";
                json_data += "\"type\": \"textBox\"";           
                json_data += "}";
                return json_data;
            });
        }

        // TODO: ADD ANY NEW ANNOTATED FIELDS TO THIS LIST
        GR_MAKE_REFLECTABLE(imGUI_textBox, out, widget_id, endpoint);

        //ZMQ subscriber to the ZMQ publisher in the dashboard_server graph for updating widgets
        void start() {

            //Create the subscriber ZMQ socket 
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            //Connect to the PUB socket in the dashboard_server
            subscriber.connect(endpoint.value);
            //Subscribe to messages with the ID of the widget
            subscriber.set(zmq::sockopt::subscribe, widget_id.value);

            //Which ever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();

        }

        void stop() {
            //Closing SUB
            if (subscriber) { subscriber.close(); };

            //unregistering from singleton
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

        [[nodiscard]] gr::work::Status processBulk(gr::OutputSpanLike auto& output) {
            const std::size_t nSamples = output.size();
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;

            //This while loop is for checking updates coming from the dashboards
            //Check if we have recieved a message frame from SUB ZMQ 
            zmq::message_t rx_frame;
            while (subscriber && subscriber.recv(rx_frame, zmq::recv_flags::dontwait)) {

                //Parse the raw message
                std::string raw_dashBoard_server_message = rx_frame.to_string();
                auto delim = raw_dashBoard_server_message.find(":");

                if (delim != std::string::npos) {

                    std::string target = raw_dashBoard_server_message.substr(0, delim);

                    //Check if its just an update from the flowgraph
                    if (target == widget_id.value) {
                        try {
                            std::string payload = raw_dashBoard_server_message.substr(delim + 1, raw_dashBoard_server_message.length());

                            if (this->on_val_update) {
                                this->on_val_update(payload);
                            }

                            //TO DO: Remove Debug Message
                            std::cout << "TextBox input received and lambda fired." << "\n";
                        }
                        catch (const std::exception& e) {
                        }
                    }
                }
            }

            // Fill dummy output port with zeroes just to consume the scheduler cycle
            std::fill_n(output.data(), nSamples, T{});
            output.publish(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

// Registering standard dummy types to sink the port
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_textBox", gr::dashboard_blocks::imGUI_textBox, [float, std::complex<float>, uint8_t, char])

#endif
