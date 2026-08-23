#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TEXTLABEL_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TEXTLABEL_HPP

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
    struct imGUI_textLabel : gr::Block<imGUI_textLabel<T>> {

        //TO DO: Add a proper description
        using Description = gr::Doc<R""(Widget for a text label, displays text set by the flowgraph, not user editable)"">;

        //TO DO: This section should contain any variables which are relevant to the user interface with the widget block
        // **Variables that can be adjusted by the user**

        //Every widget needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "id", gr::Visible> widget_id = "textLabel_default";
        
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

        //Caption thats on or to the right of the widget
        gr::Annotated<std::string, "title", gr::Visible> title = "WIDGET_NAME_1";
        
        gr::Annotated<std::string, "target_property", gr::Visible> target_property = "current_val";

        gr::Annotated<std::size_t, "max_length", gr::Visible> max_length = 256;


        //TO DO: This section should contain any variables which are not relevant to the user interface with the sink block
        // **Irrlevant to user interface**

        //Output Port (This is a dummy port which should be attached to a null sink, it never outputs anything)
        gr::PortOut<T, gr::Async> out;

        //The text currently displayed, only ever set by the flowgraph via get_external_val, not by the user
        gr::Annotated<std::string, "current_val", gr::Visible> current_val = "";

        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "ipc:///tmp/gr4_dashboard_cmds.sock";
        gr::Annotated<std::string, "zmq_SUB_dashboard_server", gr::Visible> dashboard_server = "ipc:///tmp/gr4_dashboard_data.sock";
        
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;
        zmq::socket_t subscriber;


        //These two function calls are used to update the vairiable you are controlling
        //NOTE: No on_val_update here, a label is display only, nothing flows back from the dashboard
        std::function<std::string()> get_external_val = nullptr;

        private:
        //Track whats the last value that has been published
        std::string lastPublishedValue = current_val.value;

        //Helper function for publishing variable
        //NOTE: text is variable length, so this cant memcpy sizeof(current_val.value) like the numeric widgets do,
        //has to use the actual string length instead
        void variableLength_publishCurrentVal() {
            
            //Append unique identifier for widget packets (For widgets its directly their widgets ID)
            std::string header = widget_id.value + ":";
            
            //Payload size is header + the actual text length, not sizeof(std::string)
            std::size_t payload_size = header.size() + current_val.value.size();
            zmq::message_t z_msg(payload_size);
            
            //Copy data into the buffers
            std::memcpy(z_msg.data(), header.data(), header.size());
            std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), current_val.value.data(), current_val.value.size());
            
            //Publish
            publisher.send(z_msg, zmq::send_flags::dontwait);
            //Update last published value
            lastPublishedValue = current_val.value;
        }

        public:

        imGUI_textLabel(gr::property_map initial_settings = {})
            : gr::Block<imGUI_textLabel<T>>(initial_settings)
        {
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", ";
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", ";
                json_data += "\"type\": \"textLabel\", ";           
                json_data += "\"title\": \"" + this->title.value + "\", "; 
                json_data += "\"target\": \"" + this->target_property.value + "\"";
                json_data += "}";
                return json_data;
            });
        }

        GR_MAKE_REFLECTABLE(imGUI_textLabel, out, widget_id, title, panel_name, target_property, endpoint, max_length, dashboard_server, current_val);

        //ZMQ subscriber to the ZMQ publisher in the dashboard_server graph for updating widgets
        void start() {

            //Create the subscriber ZMQ socket 
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            //Connect to the PUB socket in the dashboard_server
            subscriber.connect(endpoint.value);
            //Subscribe to messages with the ID of the widget or header SERVER
            //A label still needs to answer SERVER syncs, so a newly connected dashboard gets the current text
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

            //This while loop is for checking updates coming from the dashboards
            //A label has nothing the user can edit, this loop only ever reacts to a SERVER sync request
            zmq::message_t rx_frame;
            while (subscriber && subscriber.recv(rx_frame, zmq::recv_flags::dontwait)) {

                //Parse the raw message
                std::string raw_dashBoard_server_message = rx_frame.to_string();
                auto delim = raw_dashBoard_server_message.find(":");

                if (delim != std::string::npos) {

                    std::string target = raw_dashBoard_server_message.substr(0, delim);

                    //Check if its a SERVER sync message 
                    if (target == "SERVER") {
                        //TO DO: Remove Debug Message
                        std::cout << "I Have recieved a server message" << "\n";
                        if (publisher) {
                            variableLength_publishCurrentVal();
                            //TO DO: Remove Debug Message
                            std::cout << "Message has been published to the dashboard_server" << "\n";
                        }
                    }
                    //No widget_id branch here, a label never receives updates FROM the dashboard, only pushes text TO it
                }
            }

            //This if statement is for checking updates coming from the flowgraph itself
            //This is the only way the text on this widget ever changes
            if (this->get_external_val) {
                std::string current_flowgraph_val = this->get_external_val();

                //Max length check
                //Truncates if longer than max
                if (current_flowgraph_val.size() > max_length.value) { current_flowgraph_val.resize(max_length.value); }
                if (current_flowgraph_val != current_val.value) { current_val.value = current_flowgraph_val; }
            }

            //This piece of code will only be
            //Catches an difference between the current value and the published value
            if (current_val.value != lastPublishedValue) {
                variableLength_publishCurrentVal();
            }

            //Nothing it every actually published from the output port
            output.publish(0);
            return gr::work::Status::OK;
        
        }
    }; // namespace gr::dashboard_blocks
} // namespace gr::dashboard_blocks


GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_textLabel", gr::dashboard_blocks::imGUI_textLabel, [std::uint8_t])
#endif