#ifndef CUSTOM_ZMQ_PBU_HPP
#define CUSTOM_ZMQ_PBU_HPP

//GNU Block template
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <libwebsockets.h>
#include "../json.hpp"
#include <string.h>

namespace custom {

    using blockStatus = gr::work::Status;

//Block is selfcontained for testing

template<typename T>
struct testSink : public gr::Block<testSink<T>> {
    



    //Networking properties
    int port 9000;
    
    //slider configs
    float slider_min = 0.1f;
    float slider_max = 5.0f;

    //Here is are the 
    std::string data_source_id = "staticSin";
    std::string widget_title = "static_SinWave";
    std::string widgetType = "timerseries";


    //The block 
    GR_MAKE_REFLECTABLE(testSink);


    //Attempts to bind the socket to the address at the start, if it fails throws an error
    void start() {

    }

    void stop() {

    }


    /*
   //Pushing out the data coming into the block to be pushed out of the socket
    gr::work::Status processBulk(gr::InputSpanLike auto& input) {
        
        // 1. Ask the input directly how many items it has
        const std::size_t nSamples = input.size();

        if (nSamples == 0) {
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
        }
        
        //ZMQ requires you to create a message by giving it the data and the total number of bytes that data requires
        const size_t num_bytes = nSamples * sizeof(T);
        zmq::message_t msg(input.data() , num_bytes);

        //This actually sends the message over the socket, havent actually looked into what the flag do exactly so set to none for now
        socket.send(msg,zmq::send_flags::none);

        std::ignore = input.consume(nSamples);
        return gr::work::Status::OK;
    }
    */
};

} 

#endif 



