#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_BUTTON_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_BUTTON_HPP

//GNU Radio includes
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>

//External Dependences
#include <zmq.hpp>
#include <cstring>
#include <functional> 

namespace gr::dashboard_blocks {

    template <typename T>
    struct dep_imGUI_button : gr::Block<dep_imGUI_button<T>> {

        //TO DO: Add a proper description
        using Description = gr::Doc<R""()"">;

        //TO DO: This section should contain any variables which are relevant to the user interface with the widget block
        // **Variables that can be adjusted by the user**

        //Every widget needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "id", gr::Visible> widget_id = "button_default";
        
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";
        
        //Caption thats on or to the right of the widget
        gr::Annotated<std::string, "title", gr::Visible> title = "WIDGET_NAME_1";
        
        //TO DO: Probably should be removed 
        gr::Annotated<std::string, "target_property", gr::Visible> target_property = "current_val";

        //Button has no persisted state, this is just the payload sent out on a press
        gr::Annotated<bool, "current_val"> current_val = true;

        //Output Port
        gr::PortOut<uint8_t> out;

        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5556";
        gr::Annotated<std::string, "zmq_SUB_dashboard_server", gr::Visible> dashboard_server = "tcp://127.0.0.1:5555";
        
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;
        zmq::socket_t subscriber;

       
        //Lamdas for updating variable being controlled
        //Lamda for updating value, this is the only thing that actually ties the button to the flowgraph
        std::function<void(bool)> on_val_update = nullptr;
        //NOTE: No get_external_val here, a pulse button has no state to reconcile against

        private:
        //Helper function for publishing the press pulse out to other dashboard instances
        void publishCurrentVal() {
            
            //Append unique identifier for widget packets (For widgets its directly their widgets ID)
            std::string header = widget_id.value + ":";
            
            //Get size for header and payload as we are performing memcpy into buffers
            std::size_t payload_size = header.size() + sizeof(current_val.value);
            zmq::message_t z_msg(payload_size);
            
            //Copy data into the buffers
            std::memcpy(z_msg.data(), header.data(), header.size());
            std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), &current_val.value, sizeof(current_val.value));
            
            //Publish
            publisher.send(z_msg, zmq::send_flags::dontwait);
        }

        public:

        dep_imGUI_button(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_button<T>>(initial_settings)
        {
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", ";
                json_data += "\"type\": \"button\", ";       
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", ";
                json_data += "\"title\": \"" + this->title.value + "\", ";   
                json_data += "\"target\": \"" + this->target_property.value + "\"";
                json_data += "}";
                return json_data;
            });
        }

        
        GR_MAKE_REFLECTABLE(dep_imGUI_button, out, widget_id, panel_name, target_property, endpoint, dashboard_server, current_val);

        //ZMQ subscriber to the ZMQ publisher in the dashboard_server graph for updating widgets
        void start() {

            //Create the subscriber ZMQ socket 
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            //Connect to the PUB socket in the dashboard_server
            subscriber.connect(endpoint.value);
            //Subscribe to messages with the ID of the widget or header SERVER
            subscriber.set(zmq::sockopt::subscribe, widget_id.value);
            subscriber.set(zmq::sockopt::subscribe, "SERVER");

            publisher = zmq::socket_t(zmq_ctx,zmq::socket_type::pub);
            //Connect Publisher socket to the dashboard server 
            publisher.connect(dashboard_server.value);

            //Which ever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();

        }

        void stop() {
            //Closing SUB & PUB
            if (subscriber) { subscriber.close(); };
            if (publisher) { publisher.close(); }

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

                    //A pulse button has nothing to sync, so no SERVER branch here, only reacts to its own presses
                    if (target == widget_id.value) {
                        try {
                            std::string payload = raw_dashBoard_server_message.substr(delim + 1, raw_dashBoard_server_message.length());

                            //TO DO: Remove Debug Message
                            std::cout << "[CheckBox] Raw payload received: '" << payload << "'" << std::endl;
                            float raw_val = std::stof(payload);

                            //TO DO: Anything other than exactly "true" is treated as not a press and ignored
                            if (raw_val != 0.0f) {

                                //TO DO: Remove Debug Message
                                std::cout << "I Have recieved a button press" << "\n";

                                //Update value in the flowgraph through lamda, this is the only real effect of the button
                                if (this->on_val_update) {
                                    this->on_val_update(true);
                                }

                                //Let every other dashboard instance know a press happened
                                if (publisher) {
                                    publishCurrentVal();
                                }

                                //TO DO: Remove Debug Message
                                std::cout << "Message has been published to the dashboard_server" << "\n";
                            }
                        }
                        catch (const std::exception& e) {
                        }
                    }
                }
            }

            std::fill_n(output.data(), nSamples, static_cast<std::uint8_t>(current_val.value));
            output.publish(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks


GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_button", gr::dashboard_blocks::dep_imGUI_button, [bool])

#endif