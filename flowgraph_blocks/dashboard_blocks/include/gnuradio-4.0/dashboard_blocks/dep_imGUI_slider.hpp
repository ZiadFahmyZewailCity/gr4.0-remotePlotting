#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP

#include <chrono>
#include <cstring>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <thread>
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

        // Coordinator interrupt trigger
        std::function<void(T)> on_val_update = nullptr;
        
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
        
        //Threads & Mutexes
        //The widget essentially acts purely like a connection to the flowgraph, since it only updates a variable it has no input/output ports
        //It runs on a seperate thread purely  
        std::atomic<bool> thread_running{false};
        std::thread zmq_thread;

        public:

        dep_imGUI_slider(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_slider<T>>(initial_settings) {}

        //No ports
        //Widgets are meant to vary already existing variables 
        GR_MAKE_REFLECTABLE(dep_imGUI_slider, widget_id, target_property, endpoint, dashboard_server, current_val);

        //ZMQ subscriber to the ZMQ publisher in the dashboard_server graph for updating widgets
        void start() {

            //Create the subscriber ZMQ socket 
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            //Connect to the PUB socket in the dashboard_server
            subscriber.connect(endpoint.value);
            //Place the frequency value in the buffer of the socket
            subscriber.set(zmq::sockopt::subscribe, widget_id.value);

            //When should this wake up ? Should only PUB in the two cases of receiving PUB command from the dashboard_server
            publisher = zmq::socket_t(zmq_ctx,zmq::socket_type::pub);
            //Connect Publisher socket to the dashboard server 
            publisher.connect(dashboard_server.value);

            //Starting the thread
            thread_running = true;
            zmq_thread = std::thread(&dep_imGUI_slider::zmq_polling_widget,this);
            
        }

        void stop() {
            //Closing SUB & PUB
            if (subscriber) { subscriber.close(); };
            if (publisher) { publisher.close(); }
        }

        [[nodiscard]] gr::work::Status processBulk(gr::OutputSpanLike auto& output) {
            const std::size_t nSamples = output.size();
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;

            zmq::message_t rx_frame;
            while (subscriber && subscriber.recv(rx_frame, zmq::recv_flags::dontwait)) {
                std::string raw = rx_frame.to_string(); 
                auto delim = raw.find(':');
                if (delim != std::string::npos) {
                    try {
                        T parsed_val = static_cast<T>(std::stof(raw.substr(delim + 1)));
                        current_val.value = parsed_val;

                        // Wakes up the Flowgraph Coordinator lambda
                        if (this->on_val_update) {
                            this->on_val_update(parsed_val);
                        }
                    } catch (...) {}
                }
            }

            std::fill_n(output.data(), nSamples, current_val.value);

            output.publish(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_slider", gr::dashboard_blocks::dep_imGUI_slider, [float])

#endif