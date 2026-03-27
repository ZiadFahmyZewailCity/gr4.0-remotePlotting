cmake_minimum_required(VERSION 3.20)
project(gr4-custom CXX)

# Force C++23 to support <print> and other GR4 requirements
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Clean C++, no GNU extensions

find_package(gnuradio4 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libzmq REQUIRED IMPORTED_TARGET libzmq)

#OOT 
add_library(custom-blocklib INTERFACE)

#Custom Headers 
target_include_directories(custom-blocklib INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/> 
    $<INSTALL_INTERFACE:include/>
)

add_executable(mainFlowGraph FlowGraph/main.cpp)

# Link your custom module AND the GR4 core to the executable
target_link_libraries(mainFlowGraph PRIVATE 
    custom-blocklib 
    gnuradio4::gnuradio-core
)


target_link_libraries(custom-blocklib INTERFACE 
    gnuradio4::gnuradio-core
    gnuradio4::gnuradio-blocklib-core
    PkgConfig::libzmq
)