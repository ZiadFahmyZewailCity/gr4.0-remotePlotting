#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_<<WIDGET_NAME_UPPER>>_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_<<WIDGET_NAME_UPPER>>_HPP


//TEMPLATE FOR NEW DASHBOARD WIDGETS
//Copy this file, rename <<widget_name>> -> whatever the widget is (e.g. slider, toggle)
//and fill in the TO DO's. 

//GNU Radio includes
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/dashBoard_blocks/imGUI_management.hpp>
//TO DO: Add any extra GR4 headers this sink needs (fft.hpp / window.hpp for anything spectral, etc)

//External Dependences
#include <zmq.hpp>
#include <cstring>



namespace gr::dashboard_blocks {

    //TO DO: Rename struct 
    template <typename T>
    struct imGUI_<<widget_name>> : gr::Block<imGUI_<<widget_name>><T>> {
        
        //TO DO: give a descripiton for your widget
        using Description = gr::Doc<R""(Description)"">;


        //TO DO: This section should contain any variables which are relevant to the user interface with the widget block
        // **Variables that can be adjusted by the user**

        //Every widget needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "id", gr::Visible> widget_id = "<<widget_name>>_default";
        
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";
        
        //Caption thats on or to the right of the widget
        gr::Annotated<std::string, "title", gr::Visible> title = "WIDGET_NAME_1";
        
        gr::Annotated<std::string, "target_property", gr::Visible> target_property = "current_val";

        //This is the current value of the widget
        gr::Annotated<T, "current_val", gr::Visible> current_val = static_cast<T>(1.0);

        //TO DO: This section should contain any variables which are not relevant to the user interface with the sink block
        // **Irrlevant to user interface**

        //Output Port (This is a dummy port which should be attached to a null sink, it never outputs anything)
        gr::PortOut<T, gr::Async> out;
        

        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "ipc:///tmp/gr4_dashboard_cmds.sock";
        gr::Annotated<std::string, "zmq_SUB_dashboard_server"> dashboard_server = "ipc:///tmp/gr4_dashboard_data.sock";
        
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;
        zmq::socket_t subscriber;


        //These two function calls are used to update the vairiable you are controlling
        //On update of variable widget is tracking, call this function to update it in the flowgraph, function must be defined in the flowgraph 
        std::function<void(T)> on_val_update = nullptr;
        //Use this function to track the current state of the variable the widget is tracking the in the flowgraph, function must be defined in the flowgraph 
        //Function definition should just return variable value 
        std::function<T()> get_external_val = nullptr;

        private:
        //Track whats the last value that has been published
        T lastPublishedValue = current_val.value;

        //Helper function for publishing variable
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
            //Update last published value
            lastPublishedValue = current_val.value;
        }

        public:

        imGUI_<<widget_name>>(gr::property_map initial_settings = {})
            : gr::Block<imGUI_<<widget_name>><T>>(initial_settings)
        {
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"WIDGET\", "; //This is what is used by the dashboard to understand what widget this is
                //You will need to add adjust the code in the main of the dashboard to accomdate this new type and new widget
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", "; //Which panel is this plot associated with
                json_data += "\"title\": \"" + this->title.value + "\", "; //Text next to the widget
                json_data += "\"target\": \"" + this->target_property.value + "\"";
                // TODO: ADD ANY EXTRA JSON FIELDS FOR WIDGET-SPECIFIC PARAMS HERE
                // e.g. json_data += ", \"min\": " + std::to_string(this->min_val.value);
                json_data += "}";
                return json_data;
            });
        }

        // TODO: ADD ANY NEW ANNOTATED FIELDS TO THIS LIST
        GR_MAKE_REFLECTABLE(imGUI_<<widget_name>>, out, panel_name, widget_id, title, target_property, endpoint, dashboard_server, current_val);

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

            //This while loop is for checking updates coming from the dashboards
            //Check if we have recieved a message frame from SUB ZMQ 
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
                            publishCurrentVal();
                            //TO DO: Remove Debug Message
                            std::cout << "Message has been published to the dashboard_server" << "\n";
                        }
                    }
                    //Check if its just an update from the flowgraph
                    else if (target == widget_id.value) {
                        try {
                            std::string payload = raw_dashBoard_server_message.substr(delim + 1, raw_dashBoard_server_message.length());

                            // TODO: REPLACE PARSING LOGIC FOR THIS WIDGET'S TYPE
                            // slider (float/int): T parsed_val = static_cast<T>(std::stof(payload));
                            // toggle (bool):       T parsed_val = (payload == "1" || payload == "true");
                            // dropdown (string/id): T parsed_val = <<parse selected option>>;
                            T parsed_val = static_cast<T>(std::stof(payload));

                            if (this->on_val_update) {
                                this->on_val_update(parsed_val);
                            }

                            current_val.value = parsed_val;
                            publishCurrentVal();

                            //TO DO: Remove Debug Message
                            std::cout << "Message has been published to the dashboard_server" << "\n";
                        }
                        catch (const std::exception& e) {
                        }
                    }
                }
            }

            //This if statement is for checking updates coming from the flowgraph itself
            //Check the current state of the variable in the flowgraph
            if (this->get_external_val) {
                T current_flowgraph_val = this->get_external_val();
                if (current_flowgraph_val != current_val.value) { current_val.value = current_flowgraph_val; }
            }

            //This piece of code will only be
            //Catches an difference between the current value and the published value
            if (current_val.value != lastPublishedValue) {
                publishCurrentVal();
            }


            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

// TODO: rename registration, set correct type list (e.g. [float], [bool], [int])
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_<<widget_name>>", gr::dashboard_blocks::imGUI_<<widget_name>>, [float])

#endif