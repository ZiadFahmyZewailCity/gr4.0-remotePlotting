#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_VECTORSINK_HPP
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/DataSet.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_typeResolving.hpp>
#include <gnuradio-4.0/meta/utils.hpp>


// External dependencies
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>
#include <concepts>


namespace gr::dashboard_blocks {

    template <typename T>
    requires(gr::meta::complex_like<T> || std::floating_point<T>)
    struct imGUI_vectorSink : gr::Block<imGUI_vectorSink<T>> {

        // TO DO: Add a description
        using Description = gr::Doc<R""())"">;

        // **Variables that can be adjusted by the user**

        //Every sink needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "sink_id", gr::Visible> sink_id = "SINK_ID_1";

        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

        //Display name of the sink
        gr::Annotated<std::string, "title", gr::Visible> title = "SINK_NAME_1";
        // Axis Labeling 
        gr::Annotated<std::string, "x_axis_label", gr::Visible> x_axis_label = "x_axis";
        gr::Annotated<std::string, "y_axis_label", gr::Visible> y_axis_label = "y_axis";

        // Size of vector (Used primarily for UI setup/scaling on the dashboard side)
        gr::Annotated<size_t, "vector Size", gr::Visible> vectorSize = 256UL;
        
        gr::Annotated<std::vector<std::string>, "data_sources", gr::Visible> dataSources = std::vector<std::string>{"default_source_1"};

        gr::Annotated<size_t, "max_buffered_vectors", gr::Visible> maxBufferedVectors = 8UL;

        // **Irrlevant to user interface**

        // Input Port explicitly requires discrete vectors, not a stream of scalars
        std::vector<gr::PortIn<gr::DataSet<T>>> in;
        std::vector<std::vector<gr::DataSet<T>>> internal_buffers;

        // ZMQ related variables
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "ipc:///tmp/gr4_dashboard_data.sock";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_vectorSink(gr::property_map initial_settings = {})
            : gr::Block<imGUI_vectorSink<T>>(initial_settings)
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->sink_id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"vectorSink\", "; //This is what is used by the dashboard to understand what type of sink this is
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", "; //Which panel is this plot associated with
                json_data += "\"title\": \"" + this->title.value + "\", "; //Will be the text above the sink
                json_data += "\"x_axis_label\": \"" + this->x_axis_label.value + "\", "; //x-axis label 
                json_data += "\"y_axis_label\": \"" + this->y_axis_label.value + "\", "; //y-axis label
                json_data += "\"vectorSize\": \"" + std::to_string(this->vectorSize.value) + "\", ";
                json_data += "\"dataSources\": [";
                for (std::size_t i = 0; i < this->dataSources.value.size(); ++i) {
                    json_data += "\"" + this->dataSources.value[i] + ":" + dashboard_dtypeTag<T>() + "\"";
                    if (i + 1 < this->dataSources.value.size()) { json_data += ", "; }
                }
                json_data += "] ";
                json_data += "}";
                return json_data;
            });
        }


        GR_MAKE_REFLECTABLE(imGUI_vectorSink, in, sink_id, title, panel_name, x_axis_label, y_axis_label, dataSources, vectorSize, maxBufferedVectors, endpoint);

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

        void settingsChanged(const gr::property_map&, const gr::property_map& newSettings) {
            if (newSettings.contains("data_sources")) {
                in.resize(dataSources.value.size());
                internal_buffers.resize(dataSources.value.size());
            }
        }
        

        template<gr::InputSpanLike TInSpan>
        [[nodiscard]] gr::work::Status processBulk(std::span<TInSpan>& input_ports) {
            
 
            for (std::size_t i = 0; i < input_ports.size(); i++) {
 
                auto& inSpan = input_ports[i];
                if (inSpan.size() == 0) { continue; }
 
                std::span<const gr::DataSet<T>> in_span(inSpan.data(), inSpan.size());
                auto& buffer = internal_buffers[i];
 
                for (const auto& vec : in_span) {
 
                    //Check if the size of the vector is greater than the pre-defined user length of the vector
                    //Drops the frame if this is the case
                    //If the vector size is smaller than the vector size set by the user, the dashboard will zero out the tail
                    const std::size_t nSamples = static_cast<std::size_t>(vec.extents[0]);
                    if (nSamples > vectorSize.value) {
 
                        //Debug Message (Comment out when not needed)
                        //std::cout << "[vectorSink] Dropping frame on source " << dataSources.value[i]
                        //           << ": got vector of size " << nSamples
                        //           << " which exceeds configured vectorSize " << vectorSize.value << "\n";
                        continue; //this is a data-validity skip, not a readiness check, so it's fine under sync ports
                    }
 
                    buffer.push_back(vec);
                }
                std::ignore = inSpan.consume(in_span.size());
 
                //Drop the oldest queued vectors if this source has fallen behind publishing
                if (buffer.size() > maxBufferedVectors.value) {
                    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(buffer.size() - maxBufferedVectors.value));
                }
 
                //Publish every buffered vector, oldest first
                while (!buffer.empty()) {
 
                    const gr::DataSet<T>& vec = buffer.front();
 
                    if (publisher) {
 
                        //1) Apply header - "id:dataSource:payload" so the daemon/frontend can demux per source
                        std::string header = sink_id.value + ":" + dataSources.value[i] + ":";
 
                        //Has to be unsigned to work with the T
                        const std::size_t nExtents = static_cast<std::size_t>(vec.extents[0]);
 
                        //2) Message core - dynamically sized based on the incoming vector length
                        std::size_t payload_size = header.size() + (nExtents * sizeof(T));
                        zmq::message_t z_msg(payload_size);
 
                        //3) Cpy into zmq message buffer
                        std::memcpy(z_msg.data(), header.data(), header.size());
                        std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), vec.signal_values.data(), nExtents * sizeof(T));
 
                        //4) Send
                        publisher.send(z_msg, zmq::send_flags::dontwait);
                    }
 
                    buffer.erase(buffer.begin());
                }
            }
            return gr::work::Status::OK;
 
        }
    };
 
} // namespace gr::dashboard_blocks

// Register the block for all standard data types that might be plotted
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_vectorSink", gr::dashboard_blocks::imGUI_vectorSink, [float, double, std::complex<float>, std::complex<double>])

#endif