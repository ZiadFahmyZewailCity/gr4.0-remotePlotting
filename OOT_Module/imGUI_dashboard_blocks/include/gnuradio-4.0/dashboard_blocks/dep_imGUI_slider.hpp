#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP

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
    //REMOVE LATER (comment for my understanding), This CRTP, Its when you state that the type of the object you are inheriting from is the struct/class
    //thats inheriting it, This achieves polymorphism but staticlly
    struct dep_imGUI_slider : gr::Block<dep_imGUI_slider<T>> {

        using Description = gr::Doc<R""(Widget for slider, listens to ZMQ commands from frontend to update variable in the flowgraph)"">;

        // **Variables that can be adjusted by the user**

        //Every widget needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "id", gr::Visible> widget_id = "slider_default";
        
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";
        
        //Caption thats on or to the right of the widget
        gr::Annotated<std::string, "title", gr::Visible> title = "WIDGET_NAME_1";
        

        //TO DO: Probably remove this
        gr::Annotated<std::string, "target_property", gr::Visible> target_property = "current_val";
        //This is the current value of the widget
        gr::Annotated<T, "current_val", gr::Visible> current_val = static_cast<T>(1.0);

        // **Irrlevant to user interface**

        //Output Port (This is a dummy port which should be attached to a null sink, it never outputs anything)
        gr::PortOut<T, gr::Async> out;

        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5556";
        gr::Annotated<std::string, "zmq_SUB_dashboard_server", gr::Visible> dashboard_server = "tcp://127.0.0.1:5555";
        
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
        //Set last published value to the default
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

        dep_imGUI_slider(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_slider<T>>(initial_settings) 
        {
            // Register the widget config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", ";
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", ";
                json_data += "\"type\": \"slider\", ";
                json_data += "\"title\": \"" + this->title.value + "\", "; //Text next to the widget
                json_data += "\"target\": \"" + this->target_property.value + "\"";
                json_data += "}";
                return json_data;
            });
        }

        
        //Widgets are meant to vary already existing variables 
        GR_MAKE_REFLECTABLE(dep_imGUI_slider,out  ,widget_id, panel_name, target_property, endpoint, dashboard_server, current_val);

        //ZMQ subscriber to the ZMQ publisher in the dashboard_server graph for updating widgets
        void start() {

            //Create the subscriber ZMQ socket 
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            //Connect to the PUB socket in the dashboard_server
            subscriber.connect(endpoint.value);
            //Subscribe to messages with the ID of the widget or header SERVER
            subscriber.set(zmq::sockopt::subscribe, widget_id.value);
            subscriber.set(zmq::sockopt::subscribe, "SERVER");

            //When should this wake up ? Should only PUB in the two cases of receiving PUB command from the dashboard_server
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

            zmq::message_t rx_frame;
            while (subscriber && subscriber.recv(rx_frame, zmq::recv_flags::dontwait)) {

                //Take message, put it in a string container, find ":" delimter
                std::string raw_dashBoard_server_message = rx_frame.to_string();
                auto delim = raw_dashBoard_server_message.find(":");

                if(delim != std::string::npos){

                    //Extract the target
                    std::string target = raw_dashBoard_server_message.substr(0,delim);

                    //The second part of the cmd message is technically not needed, we could technically just remove the value part
                    //However the overhead is insignificant and this leaves room to use this communication path later on with other 
                    //messages from the server

                    //Send the latest value of the variable to the dashboard_server via the ZMQ PUB
                    if(target == "SERVER"){

                        //TO DO: Debug Message comment out eventuallly
                        std::cout << "I Have recieved a server message" << "\n";

                        if(publisher){
                            publishCurrentVal();

                            //TO DO: Debug Message comment out eventuallly
                            std::cout << "Message has been published to the dashboard_server" << "\n";
                        }

                    }

                    /*
                    //If standard message check update variable value in flowgraph  
                    //NEED TO ALSO SEND OUT AN UPDATE FOR THE OTHER FLOWGRAPHS HOWEVER THIS NEEDS TO BE TESTED A BIT MORE
                    */
                    //Take the new value of the widget and update the variable in the flowgraph
                    else if (target == widget_id.value) {

                        try{
                            //Update the variable in the flowgraph
                            //Getting the payload
                            std::string payload = raw_dashBoard_server_message.substr(delim + 1, raw_dashBoard_server_message.length());
                            //casting it to the type of the variable being controlled by the widget
                            T parsed_val = static_cast<T>(std::stof(payload));

                            //This should be the method by which we update the varible in the overall flowgraph
                            if(this->on_val_update){
                                this->on_val_update(parsed_val);
                            }

                            //Update the variable in the block itself 
                            current_val.value = parsed_val;
                            //Send new widget value over ZMQ PUB to other dashboards
                            publishCurrentVal();

                            //TO DO: Remove Debug Message
                            std::cout << "Message has been published to the dashboard_server" << "\n";

                        }
                        catch(const std::exception& e){

                        }

                    }
                }
            }


            //Lamda call for updating variable from flowgraph                 
            //Check if PTR isnt null
            if(this->get_external_val){
                //Get the current value from the flowgraph
                T current_flowgraph_val = this->get_external_val();
                //If they arent the same, set current value to be what the current value is in the flowgraph
                if(current_flowgraph_val != current_val.value) {current_val.value = current_flowgraph_val; }
            }
            
            if(current_val.value != lastPublishedValue){
                publishCurrentVal();
            }

            //Nothing it every actually published from the output port
            output.publish(0);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_slider", gr::dashboard_blocks::dep_imGUI_slider, [float])

#endif