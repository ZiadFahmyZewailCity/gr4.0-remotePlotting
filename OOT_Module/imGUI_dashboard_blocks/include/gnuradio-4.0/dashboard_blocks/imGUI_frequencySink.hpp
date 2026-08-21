#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP

//TO DO: Standardize comments across sinks


//GNU Related headers
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/DataSet.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/meta/utils.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <gnuradio-4.0/fourier/fft.hpp>
#include <gnuradio-4.0/algorithm/fourier/window.hpp>
//External dependecies
#include <iostream>
#include <magic_enum.hpp>
#include <span>
#include <vector>
#include <zmq.hpp>
#include <string>
#include <cstring>



namespace gr::dashboard_blocks {

    template <typename T>
    struct extract_real { using type = T; };

    template <typename T>
    struct extract_real<std::complex<T>> { using type = T; };


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

        // **Variables that can be adjusted by the user**

        //Every sink needs a id, this must be unique to the instantiated block as the dashboard will create a dashboard element using this ID
        gr::Annotated<std::string, "sink", gr::Visible> id = "frequency_default_id";

        //All widgets or blocks with the same panel name will be placed in the same panel
        //The panel chosen purely has an affect on the widget or blocks location, has no affect on the data it displays or affects
        gr::Annotated<std::string, "panel", gr::Visible> panel_name = "default";

        //Display name of the sink
        gr::Annotated<std::string, "title", gr::Visible> title = "FREQUENCY_SINK";
        // Axis Labeling 
        gr::Annotated<std::string, "x_axis_label", gr::Visible> x_axis_label = "x_axis";
        gr::Annotated<std::string, "y_axis_label", gr::Visible> y_axis_label = "y_axis";

        gr::Annotated<size_t, "Window Size", gr::Visible> windowSize = 1024UL;
        //TO DO: Any reason to default to something in specific, currently default to Hann
        gr::Annotated<gr::algorithm::window::Type, "Window Type", gr::Visible> windowType = gr::algorithm::window::Type::Hann;
        //CURRENTLY PLACEHOLDER DOESNT DO ANYTHING
        gr::Annotated<triggerType, "Trigger Type", gr::Visible> typeOfTrigger;
        //CURRENTLY PLACE HOLDER DOESNT DO ANYTHING
        gr::Annotated<averagingType, "Averaging Type",gr::Visible> typeOfAveraging;
        gr::Annotated<float, "Sample Rate", gr::Visible, gr::Unit<"Hz">> sampleRate = 1.0f;
        gr::Annotated<bool, "Output in dB", gr::Visible> outputInDb = true;


        // **Internal variables of the sink**
        //Input Port
        gr::PortIn<T> in;

        //The FFT block wrapped inside the imGUI block
        gr::blocks::fft::DefaultFFT<T> FFTblock{};

        //ZMQ related variables (Not to be adjusted by user)
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "ipc:///tmp/gr4_dashboard_data.sock";
        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        imGUI_frequencySink(gr::property_map initial_settings = {})
            : gr::Block<imGUI_frequencySink<T>>(initial_settings) 
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->id.value + "\", "; //Unique identifier of the block
                json_data += "\"type\": \"frequencySink\", "; //This is what is used by the dashboard to understand what type of sink this is
                json_data += "\"panel_name\": \"" + this->panel_name.value + "\", "; //Which panel is this plot associated with
                json_data += "\"title\": \"" + this->title.value + "\", "; //Will be the text above the sink
                json_data += "\"x_axis_label\": \"" + this->x_axis_label.value + "\", "; //x-axis label 
                json_data += "\"y_axis_label\": \"" + this->y_axis_label.value + "\", "; //y-axis label
                json_data += "\"windowSize\": \"" + std::to_string(this->windowSize.value) + "\", ";
                json_data += "\"samplingFreq\": \"" + std::to_string(this->sampleRate.value) + "\", ";
                json_data += "\"frequencySink_dataSource\": \"Magnitudes\" "; //TODO: Not in use currently
                json_data += "}";
                return json_data;
            });
        }

        GR_MAKE_REFLECTABLE(imGUI_frequencySink, in,id , title, panel_name, x_axis_label, y_axis_label ,windowSize, sampleRate, windowType, outputInDb, typeOfTrigger, typeOfAveraging, endpoint);

        void start() {

            publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
            publisher.connect(endpoint.value);

            //Configuring FFT block
            FFTblock.fftSize = this->windowSize;
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
            
            

            //Check if we have enough samples for
            if (input.size() < this->windowSize) { return gr::work::Status::INSUFFICIENT_INPUT_ITEMS; }

            const std::size_t nSamples = this->windowSize;

            std::span<const T> samples_frame(input.data() , nSamples);
            
            if (publisher) {


                using floattype = typename extract_real<T>::type;
                std::vector<gr::DataSet<floattype>> FFT_output(1);

                const gr::work::Status fftStatus = FFTblock.processBulk(samples_frame, std::span{FFT_output});
                if (fftStatus != gr::work::Status::OK) {
                    return fftStatus;
                }

                //Output the magnitudes
                auto& dataset = FFT_output[0];
                std::size_t num_bins = static_cast<std::size_t>(dataset.extents[0]); 
                auto* magnitudes_ptr = dataset.signal_values.data();

                //Debug Message (Comment out when not needed)
                /* 
                static int dbg = 0;
                if (dbg++ % 60 == 0) {
                    std::cout << "RAW: ";
                    for (std::size_t i = 0; i < 8; i++) std::cout << samples_frame[i] << " ";
                    std::cout << "\nMAG: ";
                    for (std::size_t i = 0; i < 8; i++) std::cout << magnitudes_ptr[i] << " ";
                    std::cout << std::endl;
                }
                */

                //TO DO: Add averaging


                //Send over ZMQ to the server

                //1) Apply header
                std::string header = id.value + ":";
                std::size_t payload_size = header.size() + (num_bins * sizeof(floattype));

                //2) Message core
                zmq::message_t z_msg(payload_size);

                //3) Cpy into zmq message buffer
                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), magnitudes_ptr, num_bins * sizeof(floattype));

                //4) Send
                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_frequencySink", gr::dashboard_blocks::imGUI_frequencySink, [float, std::complex<float>])
#endif