cmake_minimum_required(VERSION 3.20)
project(dashboard_blocks CXX)

# 1. Global Baseline Standard (Strictly C++23 for GR4 Monadic Error Handling)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 2. Global Macro Definitions
add_compile_definitions(ASIO_STANDALONE)
add_compile_definitions(ZMQ_BUILD_DRAFT_API=1)

# 3. Find Required Host Packages
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(cppzmq REQUIRED)
find_package(websocketpp REQUIRED)
find_package(gnuradio4 REQUIRED)

# =====================================================================
# Target 1: Standalone Dashboard Daemon (ZMQ <-> WebSocket Broker)
# =====================================================================
add_executable(dashboard_daemon
    src/dashBoard_daemon.cpp
    src/dashBoard_server.cpp
)

# Sandboxed to C++17 strictly for WebSocket++ legacy header compatibility
set_target_properties(dashboard_daemon PROPERTIES 
    CXX_STANDARD 17 
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(dashboard_daemon PRIVATE
    include
    src
)

target_link_libraries(dashboard_daemon PRIVATE
    Threads::Threads
    cppzmq
)

if(TARGET websocketpp::websocketpp)
    target_link_libraries(dashboard_daemon PRIVATE websocketpp::websocketpp)
elseif(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    target_include_directories(dashboard_daemon PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
endif()

# =====================================================================
# Target 2: Live GR4 Flowgraph Coordinator (Inherits C++23)
# =====================================================================
add_executable(flowgraph
    src/flowgraph.cpp
)

target_include_directories(flowgraph PRIVATE
    include
    src
)

# Explicit link hierarchy matching your verified working baseline
target_link_libraries(flowgraph PRIVATE
    gnuradio4::gnuradio-core
    gnuradio4::gnuradio-blocklib-core
    cppzmq
    fmt::fmt
    Threads::Threads
)


# =====================================================================
# Quality Assurance Suite
# =====================================================================
include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()