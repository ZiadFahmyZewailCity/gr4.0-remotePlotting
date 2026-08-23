#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TIMESERIES_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TIMESERIES_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_typeResolving.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>
#include <vector>

namespace gr::dashboard_blocks {

    template <typename T>
    struct dep_imGUI_timeSeries : gr::Block<dep_imGUI_timeSeries<T>> {

        //TO DO: Update description
        using Description = gr::Doc<R""(Ingests live DSP sample streams and broadcasts them over ZeroMQ. Preapends a topic header matching the frontend ImGui config ID.)"">;

        //Every sink needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "sink_id", gr::Visible> id = "SINK_ID_1";

        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

        //Display name of the sink
        gr::Annotated<std::string, "title", gr::Visible> title = "SINK_NAME_1";
        // Axis Labeling 
        gr::Annotated<std::string, "x_axis_label", gr::Visible> x_axis_label = "x_axis";
        gr::Annotated<std::string, "y_axis_label", gr::Visible> y_axis_label = "y_axis";

        //Each port connected to the sink should have a dataSource name added to it
        gr::Annotated<std::vector<std::string>, "data_sources", gr::Visible> dataSources = std::vector<std::string>{"default_source_1"};

        gr::Annotated<size_t, "max_buffered_samples", gr::Visible> maxBufferedSamples = 4096UL;

        // **Internal variables of the sink**

        //Input Ports (one per data source)
        std::vector<gr::PortIn<T>> in = std::vector<gr::PortIn<T>>(1);

        //Internal Buffers
        std::vector<std::vector<T>> internal_buffers = std::vector<std::vector<T>>(1);


        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "ipc:///tmp/gr4_dashboard_data.sock";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;


        dep_imGUI_timeSeries(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_timeSeries<T>>(initial_settings) 
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"timeSeries\", "; //This is what is used by the dashboard to understand what type of sink this is
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", "; //Which panel is this plot associated with
                json_data += "\"title\": \"" + this->title.value + "\", "; //Will be the text above the sink
                json_data += "\"x_axis_label\": \"" + this->x_axis_label.value + "\", "; //x-axis label 
                json_data += "\"y_axis_label\": \"" + this->y_axis_label.value + "\", "; //y-axis label
                json_data += "\"dataSources\": ["; // send over the list of data_sources
                //Itterate over the length of data sources
                for (std::size_t i = 0; i < this->dataSources.value.size(); ++i) {
                    json_data += "\"" + this->dataSources.value[i] + "\"";
                    //Place the name of the source followed by a comma
                    if (i + 1 < this->dataSources.value.size()) { json_data += ", "; }
                }
                json_data += "]";
                json_data += "}";
                return json_data;
            });
        }

        //Keeps in.size() matched to dataSources.value.size() any time data_sources is updated
        void settingsChanged(const gr::property_map&, const gr::property_map& newSettings) {
            if (newSettings.contains("data_sources")) {
                in.resize(dataSources.value.size());
                internal_buffers.resize(dataSources.value.size());
            }
        }

        GR_MAKE_REFLECTABLE(dep_imGUI_timeSeries, in, id, title, panel_name, x_axis_label, y_axis_label, dataSources, maxBufferedSamples, endpoint);

        void start() {
            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);

            //Which ever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        void stop() {
            if (publisher) publisher.close();

            //unregistering from singleton
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

        template<gr::InputSpanLike TInSpan>
        [[nodiscard]] gr::work::Status processBulk(std::span<TInSpan>& input_ports) {

            for (std::size_t i = 0; i < input_ports.size(); i++) {

                auto& inSpan = input_ports[i];

                //Skip the port if its empty
                if (inSpan.size() == 0) { continue; }

                //Copy the data of the port into the internal buffer
                auto& buffer = internal_buffers[i];
                buffer.insert(buffer.end(), inSpan.begin(), inSpan.end());
                std::ignore = inSpan.consume(inSpan.size());
                
                //Drop the oldest samples if this source has fallen behind publishing
                if (buffer.size() > maxBufferedSamples.value) {
                    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(buffer.size() - maxBufferedSamples.value));
                }

                if (publisher) {

                    const std::size_t nSamples = inSpan.size();
                    //1) Apply header - "id:dataSource:payload" so the daemon/frontend can demux per source
                    std::string header = id.value + ":" + dataSources.value[i] + ":";
                    std::size_t payload_size = header.size() + (nSamples * sizeof(T));

                    //2) Message core
                    zmq::message_t z_msg(payload_size);

                    //3) Cpy into zmq message buffer
                    std::memcpy(z_msg.data(), header.data(), header.size());
                    std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), buffer.data(), nSamples * sizeof(T));

                    //4) Send
                    publisher.send(z_msg, zmq::send_flags::dontwait);

                    //5) Clear out the temporary buffer
                    buffer.clear();
                }

            }
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_timeSeries", gr::dashboard_blocks::dep_imGUI_timeSeries, [float])

#endif