#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP

#include <chrono>
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
//Needed for std::function
#include <functional> 

namespace gr::dashboard_blocks {

    template <typename T>
    //REMOVE LATER (comment for my understanding), This CRTP, Its when you state that the type of the object you are inheriting from is the struct/class
    //thats inheriting it, This achieves polymorphism but staticlly
    struct dep_imGUI_slider : gr::Block<dep_imGUI_slider<T>> {

        using Description = gr::Doc<R""(Widget for slider, listens to ZMQ commands from frontend to update variable in the flowgraph)"">;

        //This is the unique ID that will be given to the widget by the user
        gr::Annotated<std::string, "widget_id", gr::Visible> widget_id = "slider_freq";
        gr::Annotated<std::string, "target_property", gr::Visible> target_property = "current_val";

        //Variable for PUB socket in dashboard_server
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5556";
        //Variable for the SUB socket in the dashboard_server
        gr::Annotated<std::string, "zmq_SUB_dashboard_server",gr::Visible> dashboard_server = "tcp://127.0.0.1:5555";
        
        //This is the current value of the widget
        gr::Annotated<T, "current_val", gr::Visible> current_val = static_cast<T>(1.0);

        //Output port (Place holder until i can figure out a way to update the variable in the overall flowgraph, 
        //considering having a a ptr to the variable passed to the block)
        gr::PortOut<T> out;

        //On update of variable widget is tracking, call this function to update it in the flowgraph, function must be defined in the flowgraph 
        std::function<void(T)> on_val_update = nullptr;
        //Use this function to track the current state of the variable the widget is tracking the in the flowgraph, function must be defined in the flowgraph 
        //Function definition should just return variable value 
        std::function<T()> get_external_val = nullptr;

        //Create the ZMQ context
        zmq::context_t zmq_ctx{1};
        //instantiate the ZMQ socket
        zmq::socket_t publisher;
        //Instantiate the ZMQ socket
        zmq::socket_t subscriber;
      
        //These variables dont really need to be know by a developer or the flowgraph, so setting to private
        private:
        //Set last published value to the default
        T lastPublishedValue = current_val.value;

        public:
        dep_imGUI_slider(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_slider<T>>(initial_settings) 
        {
            // Register the widget config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->widget_id.value + "\", ";
                json_data += "\"type\": \"widget\", ";
                json_data += "\"target\": \"" + this->target_property.value + "\"";
                json_data += "}";
                return json_data;
            });
        }

        //No ports
        //Widgets are meant to vary already existing variables 
        GR_MAKE_REFLECTABLE(dep_imGUI_slider,out  ,widget_id, target_property, endpoint, dashboard_server, current_val);

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
        

            //Joining threads
            thread_running = false;
            if(zmq_thread.joinable()) { zmq_thread.join(); }
            
            //Closing SUB & PUB
            if (subscriber) { subscriber.close(); };
            if (publisher) { publisher.close(); }
        }

        void zmq_polling_widget(){
            
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
                        
                        std::cout << "I Have recieved a server message" << "\n";
                        if(publisher){
                            
                            //Define the total payload size
                            std::string header = widget_id.value + ":";
                            std::size_t payload_size = header.size() + sizeof(current_val.value);

                            //Create the ZMQ message
                            zmq::message_t z_msg(payload_size);
                            
                            //Copy the header into the front of the buffer (Header is ASCII)
                            std::memcpy(z_msg.data(), header.data(), header.size());
                            //Copy the current value after the header in the buffer (Payload is binary bytes)
                            std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), &current_val.value, sizeof(current_val.value));

                            //TO DO: Consider making this a helper function so the lastPublished value is always updated
                            //Send the message to the dashboard server
                            publisher.send(z_msg, zmq::send_flags::dontwait);
                            //Always update the last published value after a publish
                            lastPublishedValue = current_val.value;

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
                            //TO DO: Use this to trigger an update if a change to the frequency value occurs within the flowgraph
                            
                            //TO DO:This is the same exact code that triggers on SYNC, consider putting into some kind of function
                            //Define the total payload size, question for mentors, is it normal to add a helper function in the private section of the block
                            std::string header = widget_id.value + ":";
                            std::size_t payload_size = header.size() + sizeof(current_val.value);

                            //Create the ZMQ message
                            zmq::message_t z_msg(payload_size);
                            
                            //Copy the header into the front of the buffer (Header is ASCII)
                            std::memcpy(z_msg.data(), header.data(), header.size());
                            //Copy the current value after the header in the buffer (Payload is binary bytes)
                            std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), &current_val.value, sizeof(current_val.value));

                            //Send the message to the dashboard server
                            publisher.send(z_msg, zmq::send_flags::dontwait);
                            //Always update the last published value after a publish
                            lastPublishedValue = current_val.value;

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
            else{
                
            }

            if(current_val.value != lastPublishedValue){

                //Define the total payload size
                std::string header = widget_id.value + ":";
                std::size_t payload_size = header.size() + sizeof(current_val.value);

                //Create the ZMQ message
                zmq::message_t z_msg(payload_size);
                
                //Copy the header into the front of the buffer (Header is ASCII)
                std::memcpy(z_msg.data(), header.data(), header.size());
                //Copy the current value after the header in the buffer (Payload is binary bytes)
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), &current_val.value, sizeof(current_val.value));

                //TO DO: Consider making this a helper function so the lastPublished value is always updated
                //Send the message to the dashboard server
                publisher.send(z_msg, zmq::send_flags::dontwait);
                //Always update the last published value after a publish
                lastPublishedValue = current_val.value;
                
            }


            std::fill_n(output.data(), nSamples, current_val.value);

        }

    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_slider", gr::dashboard_blocks::dep_imGUI_slider, [float])

#endif