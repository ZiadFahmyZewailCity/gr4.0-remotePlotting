#ifndef CUSTOM_FILE_SINK_HPP
#define CUSTOM_FILE_SINK_HPP

#include <cstddef>
#include <gnuradio-4.0/Block.hpp>
#include <fstream>
#include <string>
#include <stdexcept>

namespace custom {

    using blockStatus = gr::work::Status;


template<typename T>
struct FileSink : public gr::Block<FileSink<T>> {
    
    //Defining the blocks ports, should only be 1 input port
    gr::PortIn<T> in;

    //Defining the output file 
    std::string filename = "output.iq";
    //Tracking if we want to overwrite or append the file
    bool append = false;

    GR_MAKE_REFLECTABLE(FileSink, in, filename, append);

    // 3. Internal State
    std::ofstream fileOutput;

    //Scheduler functions
    //At the start i should just quickly check permissions to make sure we can write to the file
    void start() {

        auto mode = std::ios::binary | std::ios::out;
        if (append) {
            mode |= std::ios::app;
        }

        fileOutput.open(filename, mode);
        if (!fileOutput.is_open()) {
            throw std::runtime_error("FileSink failed to open file: " + filename);
        }
    }

    void stop() {
        //Just close the file once your done
        if (fileOutput.is_open()) {
            fileOutput.close();
        }
    }



    // 5. The Core DSP/Processing Loop
    gr::work::Status processBulk(gr::InputSpanLike auto& input) {
        
        // 1. Ask the input directly how many items it has
        const std::size_t nSamples = input.size();

        if (nSamples == 0) {
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
        }

        // 2. Grab the raw memory address directly from the input
        // Write the raw bytes to the disk with an explicit cast to satisfy the compiler
        fileOutput.write(reinterpret_cast<const char*>(input.data()), static_cast<std::streamsize>(nSamples * sizeof(T)));
        // 3. Consume the memory
        std::ignore = input.consume(nSamples);
        
        return gr::work::Status::OK;
    }
};

} 

#endif 