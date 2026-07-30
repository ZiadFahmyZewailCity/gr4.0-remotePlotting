#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP

//TO DO: Standardize comments across sinks


//GNU Related headers
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <gnuradio-4.0/fourier/fft.hpp>
#include <gnuradio-4.0/algorithm/fourier/window.hpp>
//External dependecies
#include <magic_enum.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>



namespace gr::dashboard_blocks {


    // TO DO: Figure out what type of triggers i want to implement
    enum triggerType {
        FREE_RUN,
        AUTO,
        NORMAL
    };

    // TO DO: Figure out the type of averaging i want to implement
    enum averagingType {
        NONE,
        MOVING_AVERAGE,
        MAX_HOLD
    };

    template <typename T>
    struct imGUI_frequencySink : gr::Block<imGUI_frequencySink<T>> {

        //TO DO: Create a representive description for this block
        using Description = gr::Doc<R""(FrequencySink)"">;

        //Input Port
        gr::PortIn<T> in;

        //Variables that can be adjusted by user
        gr::Annotated<std::string, "title", gr::Visible> title = "plot_1";
        gr::Annotated<size_t, "Window Size", gr::Visible> windowSize = 1024UL;
        //TO DO: Any reason to default to something in specific, currently default to Hann
        gr::Annotated<gr::algorithm::window::Type, "Window Type", gr::Visible> windowType = gr::algorithm::window::Type::Hann;
        //CURRENTLY PLACEHOLDER DOESNT DO ANYTHING
        gr::Annotated<triggerType, "Trigger Type", gr::Visible> triggerType;
        //CURRENTLY PLACE HOLDER DOESNT DO ANYTHING
        gr::Annotated<averagingType, "Averaging Type",gr::Visible> averagingType;
        gr::Annotated<float, "Sample Rate", gr::Visible, gr::Unit<"Hz">> sampleRate = 1.0f;
        gr::Annotated<bool, "Output in dB", gr::Visible> outputInDb = true;

        //The FFT block wrapped inside the imGUI block
        gr::blocks::fft::DefaultFFT<T> FFTblock{};

        //ZMQ related variables (Not to be adjusted by user)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_frequencySink(gr::property_map initial_settings = {})
            : gr::Block<imGUI_frequencySink<T>>(initial_settings) 
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->title.value + "\", ";
                json_data += "\"type\": \"frequencySink\", ";
                json_data += "\"title\": \"" + this->title.value + "\", ";
                json_data += "\"frequencySink_dataSource\": \"Magnitudes\"";
                json_data += "}";
                return json_data;
            });
        }

        GR_MAKE_REFLECTABLE(imGUI_frequencySink, in, title, windowSize, sampleRate, windowType, outputInDb, triggerType, averagingType, endpoint);

        void start() {

            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);


            

            FFTblock.fftSize = this->fftSize;
            FFTblock.sample_rate = this->sampleRate;
            FFTblock.window = std::string(magic_enum::enum_name(this->windowType.value));
            FFTblock.outputInDb = this->outputInDb;


            //Which ever block reaches this first will start dashboard server
            imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
        }

        void stop() {
            if (publisher) publisher.close();

            //unregistering from singleton
            imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
            
        }

        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
            const std::size_t nSamples = input.size();
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;

            if (publisher) {


                //Compute the FFT for the given samples 


                //Output will be a set of complex pairs


                //Apply averaging


                //Send over ZMQ to the server

                //1) Apply header
                std::string header = title.value + ":";
                std::size_t payload_size = header.size() + (nSamples * sizeof(T));

                //2) Message core
                zmq::message_t z_msg(payload_size);

                //3) Cpy into zmq message buffer
                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), input.data(), nSamples * sizeof(T));

                //4) Send
                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_frequencySink", gr::dashboard_blocks::imGUI_frequencySink, [float])

#endif