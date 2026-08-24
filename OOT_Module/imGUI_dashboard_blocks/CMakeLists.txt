cmake_minimum_required(VERSION 3.20)
project(dashboard_blocks CXX)

# Standard installation directories (must be loaded before targets use path variables)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. Global Baseline Standard (Strictly C++23 for GR4 Monadic Error Handling)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# 2. Global Macro Definitions
add_compile_definitions(ASIO_STANDALONE)
add_compile_definitions(ZMQ_BUILD_DRAFT_API=1)

# Packages
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(cppzmq REQUIRED)
find_package(websocketpp REQUIRED)
find_package(nlohmann_json REQUIRED)

# GNU Radio 4 Packages
find_package(gnuradio4 REQUIRED)
find_package(gnuradio4Library REQUIRED)
find_package(gnuradio4Blocks REQUIRED)

# =====================================================================
# Target 1: OOT Core Library (Changed to SHARED for GR4 Plugin compatibility)
# =====================================================================
add_library(imGUI_dashboard_blocks SHARED
    src/imGUI_management.cpp
)

# PUBLIC includes propagate to anything linking this library
target_include_directories(imGUI_dashboard_blocks PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(imGUI_dashboard_blocks PUBLIC
    gnuradio4::gnuradio-core
    gnuradio4::gnuradio-meta
    gnuradio4::gnuradio-blocklib-core
    cppzmq
    fmt::fmt
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# =====================================================================
# Target 2: Standalone Dashboard Daemon (ZMQ <-> WebSocket Broker)
# =====================================================================
add_executable(dashboard_daemon
    src/dashBoard_daemon.cpp
    src/dashBoard_server.cpp
)

# Injected escaped string path for runtime web assets lookup
target_compile_definitions(dashboard_daemon PRIVATE
    WEB_ASSETS_DIR=\"${CMAKE_INSTALL_FULL_DATADIR}/dashboard_blocks/web\"
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
# Target 3: Live GR4 Flowgraph Coordinator (Inherits C++23)
# =====================================================================
add_executable(flowgraph
    src/flowgraph.cpp
)

# Mirroring the exact target link order from the simple_flowgraph baseline
target_link_libraries(flowgraph PRIVATE
    gnuradio4::gnuradio-core
    gnuradio4::gnuradio-blocklib-core
    gnuradio4::gnuradio-algorithm
    gnuradio4::gr-basic
    gnuradio4::gnuradio-meta
    imGUI_dashboard_blocks
)

# =====================================================================
# Installation & Discovery Rules
# =====================================================================

# Install the shared library and export its target
install(TARGETS imGUI_dashboard_blocks
    EXPORT imGUI_dashboard_blocksTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the daemon executable to system bin
install(TARGETS dashboard_daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the public headers
install(DIRECTORY include/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate and install the targets file
install(EXPORT imGUI_dashboard_blocksTargets
    FILE imGUI_dashboard_blocksTargets.cmake
    NAMESPACE imGUI_dashboard_blocks::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashboard_blocks
)

# Generate and install the package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imGUI_dashboard_blocksConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imGUI_dashboard_blocksConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashboard_blocks
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imGUI_dashboard_blocksConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashboard_blocks
)

# Install the web assets (HTML, JS, WASM)
install(DIRECTORY web/ 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dashboard_blocks/web
)