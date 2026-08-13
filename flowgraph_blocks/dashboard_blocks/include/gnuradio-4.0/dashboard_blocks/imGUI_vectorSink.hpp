#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/DataSet.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>

// External dependencies
#include <gnuradio-4.0/meta/utils.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>


namespace gr::dashboard_blocks {

    template <typename T>
    struct imGUI_vectorSink : gr::Block<imGUI_vectorSink<T>> {

        // TO DO: Add detailed description if needed
        using Description = gr::Doc<R""())"">;

        // Input Port explicitly requires discrete vectors, not a stream of scalars
        gr::PortIn<gr::DataSet<T>> in;

        // Variables that can be adjusted by user
        gr::Annotated<std::string, "title", gr::Visible> title = "vectorSink_default";
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

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
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", ";
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


            std::span<const gr::DataSet<T>> in_span(input.data(), input.size());
            const gr::DataSet<T>& vec = in_span[in_span.size() - 1];
        

            //Check if the size of the vector is greater than the pre-defined user length of the vector 
            //Drops buffer if this is the case
            //If the vector size is smaller than the vector size set by the user, the dashboard will zero out the tail 
            const std::size_t nSamples = static_cast<std::size_t>(vec.extents[0]);
            if (nSamples > vectorSize.value) {
                std::cout << "[vectorSink] Dropping frame: got vector of size " << nSamples
                           << " which exceeds configured vectorSize " << vectorSize.value << "\n";
                std::ignore = input.consume(in_span.size());
                return gr::work::Status::OK;
            }

            //TO DO: Debug message for check what extents gives
            std::cout << "[vectorSink] extents[0]=" << vec.extents[0] << " signal_values.size()=" << vec.signal_values.size() << std::endl;

            if (publisher) {
                
                std::string header = title.value + ":";
                std::cout << "[vectorSink] got vec.extents[0]=" << vec.extents[0] << std::endl;
                
                //Has to be unsigned to work with the T
                const std::size_t nExtents = static_cast<std::size_t>(vec.extents[0]);

                // Dynamically calculate payload size based on the incoming vector length
                std::size_t payload_size = header.size() + (nExtents * sizeof(T)); 

                zmq::message_t z_msg(payload_size);

                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), vec.signal_values.data(), nExtents * sizeof(T));

                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            // Consume all the vectors rcved
            std::ignore = input.consume(in_span.size());
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

// Register the block for all standard data types that might be plotted
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_vectorSink", gr::dashboard_blocks::imGUI_vectorSink, [float, double, std::complex<float>, std::complex<double>])

#endif