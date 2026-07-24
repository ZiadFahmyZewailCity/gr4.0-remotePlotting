#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TIMESERIES_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_TIMESERIES_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/dashboard_blocks/imGUI_management.hpp>
#include <zmq.hpp>
#include <string>
#include <cstring>

namespace gr::dashboard_blocks {

    template <typename T>
    struct dep_imGUI_timeSeries : gr::Block<dep_imGUI_timeSeries<T>> {

        
        using Description = gr::Doc<R""(Ingests live DSP sample streams and broadcasts them over ZeroMQ. Preapends a topic header matching the frontend ImGui config ID.)"">;

        gr::PortIn<T> in;

        gr::Annotated<std::string, "topic_id", gr::Visible> topic_id = "plot_1";
        
        gr::Annotated<std::string, "zmq_endpoint"> endpoint = "tcp://127.0.0.1:5555";

        zmq::context_t zmq_ctx{1};
        zmq::socket_t publisher;

        dep_imGUI_timeSeries(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_timeSeries<T>>(initial_settings) 
        {
            // Register the sink config using the live variables
            imGUI_DashboardRegistry::getInstance().register_imGUI_block([this]() -> std::string {
                std::string json_data = "{";
                json_data += "\"id\": \"" + this->topic_id.value + "\", ";
                json_data += "\"type\": \"timeseries\", ";
                json_data += "\"title\": \"Real-Time Signal\", ";
                json_data += "\"data_source\": \"live_data\"";
                json_data += "}";
                return json_data;
            });
        }

        GR_MAKE_REFLECTABLE(dep_imGUI_timeSeries, in, topic_id, endpoint);

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

        [[nodiscard]] gr::work::Status processBulk(gr::InputSpanLike auto& input) {
            const std::size_t nSamples = input.size();
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;

            if (publisher) {
                std::string header = topic_id.value + ":";
                std::size_t payload_size = header.size() + (nSamples * sizeof(T));

                zmq::message_t z_msg(payload_size);
                
                std::memcpy(z_msg.data(), header.data(), header.size());
                std::memcpy(static_cast<char*>(z_msg.data()) + header.size(), input.data(), nSamples * sizeof(T));

                publisher.send(z_msg, zmq::send_flags::dontwait);
            }

            std::ignore = input.consume(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_timeSeries", gr::dashboard_blocks::dep_imGUI_timeSeries, [float])

#endif