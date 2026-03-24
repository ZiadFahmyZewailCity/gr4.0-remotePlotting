cmake_minimum_required(VERSION 3.20)
project(sinWaveTest CXX)

# Force C++23 to support <print> and other GR4 requirements
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Clean C++, no GNU extensions

find_package(gnuradio4 REQUIRED)
find_package(PkgConfig REQUIRED)


pkg_check_modules(libzmq REQUIRED IMPORTED_TARGET libzmq)


add_executable(mainFlowGraph main.cpp)

target_link_libraries(mainFlowGraph 
    PRIVATE 
        gnuradio4::gnuradio-core
        gnuradio4::gnuradio-blocklib-core
        PkgConfig::libzmq
)