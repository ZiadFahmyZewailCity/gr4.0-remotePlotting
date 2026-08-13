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

        //TO DO: Give this a proper one line description, same pattern as the other sinks
        using Description = gr::Doc<R""(SINKNAME)"">;

        //Input Port
        //TO DO: Confirm T is the right sample type for this sink (float / std::complex<float> / etc)
        //Constellation for example needs to be complex only, so a static_assert here might be worth adding
        gr::PortIn<T> in;

        //Variables that can be adjusted by user
        //Every sink needs a title/id, this is what ties it to the frontend widget + the ZMQ topic header
        gr::Annotated<std::string, "title", gr::Visible> title = "SINKNAME_1";
        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

        //TO DO: Add whatever settings this sink actually needs, following the same Annotated<> pattern
        //e.g. windowSize for anything that batches N samples before publishing
        //e.g. sampleRate if the frontend needs to label an axis in Hz
        //e.g. historyDepth if its a scrolling/rolling display (frontend-only hint, backend doesnt need to store history)

        //ZMQ related variables (Not to be adjusted by user)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_SINKNAME(gr::property_map initial_settings = {})
            : gr::Block<imGUI_SINKNAME<T>>(initial_settings)
        {
            // Register the sink config using the live variables
            // This is what ends up in config.json and tells dashboard.js how to draw the widget
            //TO DO: Fill in the fields the frontend renderer for this "type" actually needs
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->title.value + "\", ";
                json_data += "\"type\": \"SINKNAME\", "; //TO DO: pick the type string dashboard.js will switch on
                json_data += "\"title\": \"" + this->title.value + "\" ";
                //TO DO: append any extra metadata fields here, comma separated, same string-building style as the others
                json_data += "}";
                return json_data;
            });
        }

        //TO DO: List every member here that GR4 needs to know about (in port + all Annotated<> settings)
        GR_MAKE_REFLECTABLE(imGUI_SINKNAME, in, title, endpoint);

        void start() {

            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);

            //TO DO: Any one-time setup goes here (configuring an FFT block, sizing internal buffers, etc)

            //Which ever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        void stop() {
            if (publisher) publisher.close();

            //unregistering from singleton
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        }

        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {

            //TO DO: Decide how many samples this sink needs before it can publish a frame
            //Timeseries just takes whatever it gets each call, frequencySink/vector/constellation
            //wait until they have a full windowSize/vectorSize/pointsPerFrame worth of samples
            const std::size_t nSamples = input.size(); //TO DO: replace with a fixed size + INSUFFICIENT_INPUT_ITEMS check if needed
            if (nSamples == 0) { return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; }

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