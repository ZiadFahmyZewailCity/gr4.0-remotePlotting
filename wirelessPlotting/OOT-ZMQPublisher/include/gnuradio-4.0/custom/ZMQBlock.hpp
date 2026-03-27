#ifndef CUSTOM_ZMQ_PBU_HPP
#define CUSTOM_ZMQ_PBU_HPP

//ZMQ Library
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <zmq.hpp>
//GNU Block template
#include <gnuradio-4.0/Block.hpp>


namespace custom {

    using blockStatus = gr::work::Status;



template<typename T>
struct ZMQBlock : public gr::Block<ZMQBlock<T>> {
    
    //Defining the blocks ports, should only be 1 input port
    gr::PortIn<T> in;

    //Address to be published to
    std::string address = "tcp://127.0.0.1:5555";



    //The block and its inputs
    GR_MAKE_REFLECTABLE(ZMQBlock, in, address);

    //ZMQ requires you to define an instance of it managed by its context 
    zmq::context_t context{1};
    //Attaching a socket to the context and setting it to be for publishing
    zmq::socket_t socket{context,zmq::socket_type::pub};

    //Attempts to bind the socket to the address at the start, if it fails throws an error
    void start() {
        try {
            socket.bind(address);
        } catch (const zmq::error_t& e)
        {
            throw std::runtime_error("ZMQ binding to socket failed");
        }

    }

    void stop() {
        //Disconnect ZMQ context from socket
        socket.unbind(address);
    }



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
};

} 

#endif 




