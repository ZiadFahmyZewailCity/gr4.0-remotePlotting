#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SINKNAME_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SINKNAME_HPP

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
//TO DO: Add any extra GR4 headers this sink needs (fft.hpp / window.hpp for anything spectral, etc)

//External dependencies
#include <zmq.hpp>
#include <string>
#include <cstring>
#include <span>
#include <vector>

namespace gr::dashboard_blocks {

    //TO DO: Rename struct + Description doc string to match the sink
    template <typename T>
    struct imGUI_SINKNAME : gr::Block<imGUI_SINKNAME<T>> {

        //TO DO: Give a description
        using Description = gr::Doc<R""(SINKNAME)"">;

        //TO DO: This section should contain any variables which are relevant to the user interface with the sink block
        // **Variables that can be adjusted by the user**
        
        
        //Every sink needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "sink_id", gr::Visible> sink_id = "SINK_ID_1";
        //Display name of the sink
        gr::Annotated<std::string, "title", gr::Visible> title = "SINK_NAME_1";
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";


        //TO DO: This section should contain any variables which are not relevant to the user interface with the sink block
        // **Internal variables of the sink**

        //Input Port
        gr::PortIn<T> in;

        //ZMQ related variables (Connection to server process)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;



        //Block constructor
        imGUI_SINKNAME(gr::property_map initial_settings = {})
            : gr::Block<imGUI_SINKNAME<T>>(initial_settings)
        {
            // Register the sink config using the live variables
            // This is what ends up in config.json and tells dashboard.js how to draw the widget
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->sink_id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"SINKNAME\", "; //This is what is used by the dashboard to understand what type of sink this is
                json_data += "\"title\": \"" + this->title.value + "\" "; //Will be the text above the sink
                //TO DO: append any extra metadata fields here, comma separated, you will need to adjust the code in the dashboard in order to utilize this new field
                json_data += "}";
                return json_data;
            });
        }

        //TO DO: List every member here that GR4 needs to know about (in port + all Annotated<> settings)
        GR_MAKE_REFLECTABLE(imGUI_SINKNAME, in, title, endpoint);


        //Function that runs on start of the block
        void start() {

            //This connects the block to a seperate process (IPC) which is a server that handles the communication with the dashboard in the browser
            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);

            //TO DO: Any one-time setup goes here (configuring an FFT block, sizing internal buffers, etc)




            //Which ever block reaches this first will start dashboard server process
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        //Function that runs upon shutting down of the flowgraph
        void stop() {

            //Close connection to the server
            if (publisher) publisher.close();

            //Required to be done to allow for proper closing of the server
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

        //This is the part of the block thats actually runnin during the running of the flowgraph
        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {

            //TO DO: Decide how many samples this sink needs before it can publish a frame
            //Timeseries just takes whatever it gets each call, frequencySink/vector/constellation
            //wait until they have a full windowSize/vectorSize/pointsPerFrame worth of samples
            
            
            //TO DO: decide how many samples are needed in order to run the rest of the code
            //This could just be a simple check that the number of samples is not zero, or a check of if the number of samples has passed a threadhold
            if (input.size() == 0) { return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; }

            //The number of samples which will be used 
            size_t nSamples = input.size();
            std::span<const T> samples_frame(input.data(), nSamples);

            if (publisher) {

                //TO DO: Do whatever processing this sink needs (FFT, magnitude, repacking, etc)
                //If no processing is needed (timeseries/vector/constellation) the raw samples can be
                //memcpy'd straight in, since std::complex<T> is layout compatible with T[2]

                //1) Apply header - every message on the wire is "id:payload", daemon splits on the first ':'
                std::string header = title.value + ":";
                std::size_t payload_size = header.size() + (nSamples * sizeof(T)); //TO DO: adjust size if payload isnt just raw samples

                //2) Message core
                zmq::message_t z_msg(payload_size);

                //3) Cpy into zmq message buffer
                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), samples_frame.data(), nSamples * sizeof(T));
                //TO DO: if payload isnt raw samples (eg processed magnitudes) memcpy the processed buffer instead

                //4) Send
                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

//TO DO: Register the block, list every T this sink should be instantiable for
GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_SINKNAME", gr::dashboard_blocks::imGUI_SINKNAME, [float])

#endif