#ifndef GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP
#define GNURADIO_DASHBOARDBLOCKS_DEP_IMGUI_SLIDER_HPP

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/annotated.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <zmq.hpp>
#include <string>
#include <algorithm>
#include <functional> // Required for std::function

namespace gr::dashboard_blocks {

    template <typename T>
    struct dep_imGUI_slider : gr::Block<dep_imGUI_slider<T>> {

        using Description = gr::Doc<R""(
            Listens to the ZeroMQ command bus for frontend ImGui widget adjustments.
            Outputs the current widget state as a continuous streaming DC signal.
        )"">;

        gr::PortOut<T> out;

        gr::Annotated<std::string, "widget_id", gr::Visible> widget_id = "slider_freq";
        gr::Annotated<std::string, "zmq_endpoint", gr::Visible> endpoint = "tcp://127.0.0.1:5556";
        gr::Annotated<T, "current_val", gr::Visible> current_val = static_cast<T>(1.0);

        // Coordinator interrupt trigger
        std::function<void(T)> on_val_update = nullptr;

        zmq::context_t zmq_ctx{1};
        zmq::socket_t subscriber;

        dep_imGUI_slider(gr::property_map initial_settings = {})
            : gr::Block<dep_imGUI_slider<T>>(initial_settings) {}

        GR_MAKE_REFLECTABLE(dep_imGUI_slider, out, widget_id, endpoint, current_val);

        void start() {
            subscriber = zmq::socket_t(zmq_ctx, zmq::socket_type::sub);
            subscriber.connect(endpoint.value);

            subscriber.set(zmq::sockopt::subscribe, widget_id.value);
        }

        void stop() {
            if (subscriber) subscriber.close();
        }

        [[nodiscard]] gr::work::Status processBulk(gr::OutputSpanLike auto& output) {
            const std::size_t nSamples = output.size();
            if (nSamples == 0) return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;

            zmq::message_t rx_frame;
            while (subscriber && subscriber.recv(rx_frame, zmq::recv_flags::dontwait)) {
                std::string raw = rx_frame.to_string(); 
                auto delim = raw.find(':');
                if (delim != std::string::npos) {
                    try {
                        T parsed_val = static_cast<T>(std::stof(raw.substr(delim + 1)));
                        current_val.value = parsed_val;

                        // Wakes up the Flowgraph Coordinator lambda
                        if (this->on_val_update) {
                            this->on_val_update(parsed_val);
                        }
                    } catch (...) {}
                }
            }

            std::fill_n(output.data(), nSamples, current_val.value);

            output.publish(nSamples);
            return gr::work::Status::OK;
        }
    };

} // namespace gr::dashboard_blocks

GR_REGISTER_BLOCK("gr::dashboard_blocks::dep_imGUI_slider", gr::dashboard_blocks::dep_imGUI_slider, [float])

#endif