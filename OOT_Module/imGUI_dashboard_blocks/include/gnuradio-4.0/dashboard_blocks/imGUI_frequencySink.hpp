#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_FREQUENCYSINK_HPP

//TO DO: Standardize comments across sinks


//GNU Related headers
#include <concepts>
#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/DataSet.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/meta/utils.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_typeResolving.hpp>
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
    //Frequency sink can take in complex types or floats
    requires(gr::meta::complex_like<T> || std::floating_point<T>)
    struct imGUI_frequencySink : gr::Block<imGUI_frequencySink<T>> {

        //TO DO: Add proper comment
        using floatType = scalar_type_t<T>;

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

        gr::Annotated<std::vector<std::string>, "data_sources", gr::Visible> dataSources = std::vector<std::string>{"default_source_1"};
        
        gr::Annotated<size_t, "Window Size", gr::Visible> windowSize = 1024UL;

        gr::Annotated<gr::algorithm::window::Type, "Window Type", gr::Visible> windowType = gr::algorithm::window::Type::Hann;
        //Create a second data source for each port that publishes the phase of the input
        gr::Annotated<bool, "Output Phase", gr::Visible> publishPhase = false;
        gr::Annotated<bool, "Unwrap Phase", gr::Visible> unwrapPhase = false;

        //Not implemented
        gr::Annotated<triggerType, "Trigger Type", gr::Visible> typeOfTrigger;
        gr::Annotated<averagingType, "Averaging Type",gr::Visible> typeOfAveraging;
        gr::Annotated<float, "Sample Rate", gr::Visible, gr::Unit<"Hz">> sampleRate = 1.0f;
        gr::Annotated<bool, "Output in dB", gr::Visible> outputInDb = true;
        gr::Annotated<bool, "Output Phase in Degrees", gr::Visible> outputPhaseInDeg = false;

        gr::Annotated<size_t, "max_buffered_samples", gr::Visible> maxBufferedSamples = windowSize.value * 4;


        // **Internal variables of the sink**
        //Input Ports (one per data source) - back to vector ports now that scheduling is
        //confirmed working in isolation. This tests multi-source specifically, separate
        //from the still-open question of what in the full graph was blocking scheduling.
        std::vector<gr::PortIn<T>> in = std::vector<gr::PortIn<T>>(1);

        //Internal Buffers (one per port)
        std::vector<std::vector<T>> internal_buffers = std::vector<std::vector<T>>(1);

        //A single shared FFT instance - windowSize/windowType/sampleRate/outputInDb are all
        //single (not per-source) fields, so every source already uses identical FFT config.
        //Called sequentially once per port per call, never concurrently.
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
                json_data += "\"dataSources\": [";
                for (std::size_t i = 0; i < this->dataSources.value.size(); ++i) {
                    json_data += "\"" + this->dataSources.value[i] + ":" + dashboard_dtypeTag<T>() + "\"";
                    if (publishPhase.value) {
                        json_data += ", \"" + this->dataSources.value[i] + "_phase:" + dashboard_dtypeTag<T>() + "\"";
                    }
                    if (i + 1 < this->dataSources.value.size()) { json_data += ", "; }
                }
                json_data += "] ";
                json_data += "}";
                return json_data;
            });
        }



    GR_MAKE_REFLECTABLE(imGUI_frequencySink, in, id, title, panel_name, x_axis_label, y_axis_label, dataSources, windowSize, sampleRate, windowType, outputInDb, publishPhase, unwrapPhase, outputPhaseInDeg, typeOfTrigger, typeOfAveraging, maxBufferedSamples, endpoint);
    
    void start() {

        publisher = zmq::socket_t(zmq_ctx, zmq::socket_type::pub);
        publisher.connect(endpoint.value);

        //Configuring the shared FFT block
        FFTblock.fftSize = windowSize.value;
        FFTblock.sample_rate = sampleRate.value;
        FFTblock.window = std::string(magic_enum::enum_name(windowType.value));
        FFTblock.outputInDb = outputInDb.value; 
        FFTblock.outputInDeg = outputPhaseInDeg.value; 
        FFTblock.unwrapPhase = unwrapPhase.value; 

        FFTblock.settingsChanged({}, {{"fftSize", gr::Size_t(windowSize.value)},
                              {"window",  FFTblock.window.value}});

        //debug message, uncomment when needed
        /*
        std::cerr << "[freq_sink::start] id=" << id.value
                    << " in.size()=" << in.size()
                    << " internal_buffers.size()=" << internal_buffers.size()
                    << " dataSources.size()=" << dataSources.value.size()
                    << std::endl;
        */
        //Which ever block reaches this first will start dashboard server
        imGUI_DashboardRegistry::getInstance().boot_dashboardServer_Once();
    }

    void stop() {
        if (publisher) publisher.close();

        //unregistering from singleton
        imGUI_DashboardRegistry::getInstance().unregisterBlockAndTeardown();
        
    }

        void settingsChanged(const gr::property_map&, const gr::property_map& newSettings) {
            
            if (newSettings.contains("data_sources")) {
                in.resize(dataSources.value.size());
                internal_buffers.resize(dataSources.value.size());
            }
            if (newSettings.contains("sampleRate"))          { FFTblock.sample_rate = sampleRate.value; }
            if (newSettings.contains("outputInDb"))          { FFTblock.outputInDb = outputInDb.value; }
            if (newSettings.contains("outputPhaseInDeg"))    { FFTblock.outputInDeg = outputPhaseInDeg.value; }
            if (newSettings.contains("unwrapPhase"))         { FFTblock.unwrapPhase = unwrapPhase.value; }

            if (newSettings.contains("windowSize")) {
                FFTblock.fftSize = windowSize.value;
                FFTblock.settingsChanged({}, {{"fftSize", gr::Size_t(windowSize.value)}});
            }
            if (newSettings.contains("windowType")) {
                FFTblock.window = std::string(magic_enum::enum_name(windowType.value));
                FFTblock.settingsChanged({}, {{"window", FFTblock.window.value}});
            }

        }

        template<gr::InputSpanLike TInSpan>
        [[nodiscard]] gr::work::Status processBulk(std::span<TInSpan>& input_ports) {

            //Debug message, uncomment when needed
            /*
            static std::size_t enter_count = 0;
            enter_count++;
            if (enter_count % 50 == 1) {
                std::cerr << "[freq_sink::processBulk] ENTER #" << enter_count
                          << " id=" << id.value
                          << " input_ports.size()=" << input_ports.size() << std::endl;
            }
            */

            for (std::size_t i = 0; i < input_ports.size(); i++) {

                auto& inSpan = input_ports[i];
                auto& buffer = internal_buffers[i];

                const std::size_t nSamples = inSpan.size();
                if (nSamples == 0) { continue; }

                //Copy the data of the port into its internal buffer
                buffer.insert(buffer.end(), inSpan.data(), inSpan.data() + nSamples);
                std::ignore = inSpan.consume(nSamples);

                //Drop the oldest samples if this source has fallen behind publishing
                if (buffer.size() > maxBufferedSamples.value) {
                    buffer.erase(buffer.begin(),
                        buffer.begin() + static_cast<std::ptrdiff_t>(buffer.size() - maxBufferedSamples.value));
                }

                std::size_t offset = 0;
                //Itterate through the buffer taking window sized chunks
                while (offset + windowSize.value <= buffer.size()) {

                    std::span<const T> samples_frame(buffer.data() + offset, windowSize.value);
                    if (publisher) {

                        std::vector<gr::DataSet<floatType>> FFT_output(1);

                        const gr::work::Status fftStatus = FFTblock.processBulk(samples_frame, std::span{FFT_output});
                        if (fftStatus != gr::work::Status::OK) {
                            //Drop just this chunk and keep going, rather than abandoning the call
                            offset += windowSize.value;
                            continue;
                        }

                        auto& dataset = FFT_output[0];
                        std::size_t num_bins = static_cast<std::size_t>(dataset.extents[0]);
                        auto* magnitudes_ptr = dataset.signal_values.data();


                        //Debug message, uncomment when needed
                        /*
                        static std::vector<std::size_t> publish_counts;
                        if (publish_counts.size() <= i) publish_counts.resize(i + 1, 0);
                        publish_counts[i]++;
                        if (publish_counts[i] % 20 == 1) {
                            std::cerr << "[freq_sink::publish] source=" << dataSources.value[i]
                                      << " #" << publish_counts[i]
                                      << " windowSize=" << windowSize.value
                                      << " num_bins=" << num_bins
                                      << " first2=[" << (num_bins > 0 ? magnitudes_ptr[0] : 0.f)
                                      << ", " << (num_bins > 1 ? magnitudes_ptr[1] : 0.f) << "]"
                                      << std::endl;
                        }
                        */

                        //TO DO: Add averaging

                        //1) Apply header - id:dataSource:payload
                        std::string header = id.value + ":" + dataSources.value[i] + ":";
                        std::size_t payload_size = header.size() + (num_bins * sizeof(floatType));

                        //2) Message core
                        zmq::message_t z_msg(payload_size);

                        //3) Cpy into zmq message buffer
                        std::memcpy(z_msg.data(), header.data(), header.size());
                        std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), magnitudes_ptr, num_bins * sizeof(floatType));

                        //4) Send
                        publisher.send(z_msg, zmq::send_flags::dontwait);


                        if (publishPhase.value) {
                            auto* phase_ptr = magnitudes_ptr + num_bins;   
                            std::string phaseHeader = id.value + ":" + dataSources.value[i] + "_phase:";
                            std::size_t phaseSize = phaseHeader.size() + (num_bins * sizeof(floatType));
                            zmq::message_t phase_msg(phaseSize);
                            std::memcpy(phase_msg.data(), phaseHeader.data(), phaseHeader.size());
                            std::memcpy(static_cast<char*>(phase_msg.data()) + phaseHeader.size(), phase_ptr, num_bins * sizeof(floatType));
                            publisher.send(phase_msg, zmq::send_flags::dontwait);
                        }
                    }

                    offset += windowSize.value;
                }

                //Erase all the samples we processed from this port's buffer
                if (offset > 0) {
                    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(offset));
                }
            }

            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::imGUI_frequencySink", gr::dashboard_blocks::imGUI_frequencySink, [float, double, std::complex<float>, std::complex<double>])
#endif