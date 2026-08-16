#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_CONSTELLATIONSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_CONSTELLATIONSINK_HPP

//TO DO: Standardize comments across sinks

//TEMPLATE FOR NEW DASHBOARD SINKS
//Copy this file, rename SINKNAME -> whatever the sink is (e.g. vectorSink, constellationSink, waterfallSink)
//and fill in the TO DO's. Delete this note once done.

//GNU Related headers
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>

//External dependencies
#include <gnuradio-4.0/meta/utils.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>

namespace gr::dashboard_blocks {

    template <typename T>
    struct imGUI_constellationSink : gr::Block<imGUI_constellationSink<T>> {

        //TO DO: Give this a proper one line description, same pattern as the other sinks
        using Description = gr::Doc<R""(SINKNAME)"">;



        //Variables that can be adjusted by user

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

        
        // **Control Plotting**

        //Control plotted points:

        //Controls if the points plotted should be the latest rcved or if the they should remain until buffer is full and are overwritten 
        //TO DO: Add to config file and add persistence 
        //Currently not working, just plots latest plots
        gr::Annotated<bool, "presistance_on" ,gr::Visible> state_presistance  = true;
        //Total number of points that can be plotted at any given time
        gr::Annotated<size_t, "numberOfPoints", gr::Visible> numberOfPoints = 256UL;


        // **Irrlevant to user interface**

        //Input Port
        //TO DO: Add a static assert to force complex
        gr::PortIn<std::complex<T>> in;

        //ZMQ related variables (Not to be adjusted by user)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_constellationSink(gr::property_map initial_settings = {})
            : gr::Block<imGUI_constellationSink<T>>(initial_settings)
        {
            // Register the sink config using the live variables
            // This is what ends up in config.json and tells dashboard.js how to draw the widget
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->sink_id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"constellationSink\", "; //This is what is used by the dashboard to understand what type of sink this is
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", "; //Which panel is this plot associated with
                json_data += "\"title\": \"" + this->title.value + "\", "; //Will be the text above the sink
                json_data += "\"x_axis_label\": \"" + this->x_axis_label.value + "\", "; //x-axis label 
                json_data += "\"y_axis_label\": \"" + this->y_axis_label.value + "\", "; //y-axis label
                json_data += "\"presistance_on\": ";
                json_data += (this->state_presistance.value ? "true" : "false");
                json_data += ", \"numberOfPoints\": \"" + std::to_string(this->numberOfPoints.value) + "\" ";
                json_data += "}";
                return json_data;
            });
        }

        //TO DO: Remove the max and min options here
        GR_MAKE_REFLECTABLE(imGUI_constellationSink, in,sink_id, title ,panel_name, x_axis_label, y_axis_label, endpoint, state_presistance, numberOfPoints);

        void start() {

            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        void stop() {

            if (publisher) publisher.close();
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {

            //Constellation will only send in batches of the size of the number of points
            if (input.size() < this->numberOfPoints) { return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; }
            const std::size_t nSamples = this->numberOfPoints; //TO DO: replace with a fixed size + INSUFFICIENT_INPUT_ITEMS check if needed


            std::span<const std::complex<T>> samples_frame(input.data(), nSamples);

            if (publisher) {

                //1) Apply header - every message on the wire is "id:payload", daemon splits on the first ':'
                std::string header = sink_id.value + ":";
                std::size_t payload_size = header.size() + (nSamples * sizeof(std::complex<T>));

                //2) Message core
                zmq::message_t z_msg(payload_size);

                //3) Cpy into zmq message buffer
                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), samples_frame.data(), nSamples * sizeof(std::complex<T>));
                //TO DO: if payload isnt raw samples (eg processed magnitudes) memcpy the processed buffer instead

                //4) Send
                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks


GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_constellationSink", gr::dashboard_blocks::imGUI_constellationSink, [float])

#endif