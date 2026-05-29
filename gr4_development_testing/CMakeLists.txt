cmake_minimum_required(VERSION 3.20)
project(StaticFlowGraphTest CXX)

# Use the C++ 23 Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# --- Tell websocketpp to use standalone ASIO instead of Boost ---
add_compile_definitions(ASIO_STANDALONE)

# --- Find required packages ---
find_package(gnuradio4 REQUIRED)
find_package(fmt REQUIRED) 
find_package(Threads REQUIRED)

# --- Compile the flowgraph ---
add_executable(staticFlowGraph flowgraphs/POCFlowGraph.cpp)

# --- Tell CMake where to find your custom OOT block headers ---
target_include_directories(staticFlowGraph PRIVATE POC_Blocks/include)

# --- Link libraries ---
target_link_libraries(staticFlowGraph 
    PRIVATE 
        gnuradio4::gnuradio-core
        gnuradio4::gnuradio-blocklib-core
        fmt::fmt
        Threads::Threads
)