#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>

// External dependencies
#include <gnuradio-4.0/meta/utils.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>
#include <vector>

namespace gr::dashboard_blocks {

    template <typename T>
    struct imGUI_vectorSink : gr::Block<imGUI_vectorSink<T>> {

        //TO DO: Add detailed description if needed
        using Description = gr::Doc<R""())"">;

        // Input Port explicitly requires discrete vectors, not a stream of scalars
        gr::PortIn<std::vector<T>> in;

        // Variables that can be adjusted by user
        gr::Annotated<std::string, "title", gr::Visible> title = "vectorSink_default";

        // Size of vector (Used primarily for UI setup/scaling on the dashboard side)
        gr::Annotated<size_t, "vector Size", gr::Visible> vectorSize = 256UL;
        
        // ZMQ related variables
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_vectorSink(gr::property_map initial_settings = {})
            : gr::Block<imGUI_vectorSink<T>>(initial_settings)
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->title.value + "\", ";
                json_data += "\"type\": \"vector\", "; 
                json_data += "\"title\": \"" + this->title.value + "\", ";
                json_data += "\"vectorSize\": \"" + std::to_string(this->vectorSize.value) + "\" ";
                json_data += "}";
                return json_data;
            });
        }

        
        GR_MAKE_REFLECTABLE(imGUI_vectorSink, in, title, vectorSize, endpoint);

        void start() {
            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);

            // Whichever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        void stop() {
            if (publisher) publisher.close();

            // Unregistering from singleton
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

         
        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
            
            // Check if we have at least one vector item
            if (input.size() == 0) { 
                return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; 
            }


            
            // Create a view of the incoming vectors and grab the first one
            std::span<const std::vector<T>> in_span(input.data(), input.size());
            const std::vector<T>& vec = in_span[0]; 

            std::cout << "[vectorSink] got vec.size()=" << vec.size() << std::endl;

            if (publisher) {
                
                std::string header = title.value + ":";
                std::cout << "[vectorSink] got vec.size()=" << vec.size() << std::endl;
                
                // Dynamically calculate payload size based on the incoming vector length
                std::size_t payload_size = header.size() + (vec.size() * sizeof(T)); 

                zmq::message_t z_msg(payload_size);

                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), vec.data(), vec.size() * sizeof(T));

                

                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            // Tell the scheduler we successfully consumed 1 vector item
            std::ignore = input.consume(1);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

// Register the block for all standard data types that might be plotted
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_vectorSink", gr::dashboard_blocks::imGUI_vectorSink, [float, double, std::complex<float>, std::complex<double>])

#endif