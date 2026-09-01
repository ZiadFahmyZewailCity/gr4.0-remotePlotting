#### What is being built
cmake_minimum_required(VERSION 3.20)
project(gr4-remotePlotting)

#CMAKE Modules for installation
#destination-path variables
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

#### What does each target need to be built 
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

## Build target Management singelton & Blocks
add_library(imGUI_dashBoard_blocks SHARED 
    src/imGUI_management.cpp
)

target_compile_definitions(imGUI_dashBoard_blocks PUBLIC ZMQ_BUILD_DRAFT_API=1)

# Directories which the singleton needs
target_include_directories(imGUI_dashBoard_blocks PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# singelton dependecies
target_link_libraries(imGUI_dashBoard_blocks PUBLIC
    gnuradio4::gnuradio-core
    gnuradio4::gnuradio-meta
    gnuradio4::gnuradio-blocklib-core
    cppzmq
    fmt::fmt
    Threads::Threads
    nlohmann_json::nlohmann_json
)

## Build target dashBoard daemon
add_executable(dashBoard_daemon 
    src/dashBoard_daemon.cpp 
    src/dashBoard_server.cpp
)

# WASM files path (In share directory)
target_compile_definitions(dashBoard_daemon PRIVATE
    WEB_ASSETS_DIR=\"${CMAKE_INSTALL_FULL_DATADIR}/dashBoard_blocks/web\"
    ASIO_STANDALONE
)

# WebSocket++ not compatible with latest Cpp, Using cpp 17
set_target_properties(dashBoard_daemon PROPERTIES 
    CXX_STANDARD 17 
    CXX_STANDARD_REQUIRED ON
)

# Directories which the dashBoard_daemon needs
target_include_directories(dashBoard_daemon PRIVATE
    include
    src
)

# dashBoard daemon dependecies
target_link_libraries(dashBoard_daemon PRIVATE
    Threads::Threads
    cppzmq
)

# A precaution i read that should be done with libraries which are header only
if(TARGET websocketpp::websocketpp)
    target_link_libraries(dashBoard_daemon PRIVATE websocketpp::websocketpp)
elseif(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    target_include_directories(dashBoard_daemon PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
endif()


#### What does each output go when someone install its

# Install .so file and register consumers should look at /include
install(TARGETS imGUI_dashBoard_blocks
    EXPORT imGUI_dashBoard_blocksTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install daemon executable to /bin
install(TARGETS dashBoard_daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Cpy header to /include
install(DIRECTORY include/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


#### How does someone else project find and use what is installed

# Generate the script to recreate teh imGUI_dashBoard_blocks target
install(EXPORT imGUI_dashBoard_blocksTargets
    FILE imGUI_dashBoard_blocksTargets.cmake
    NAMESPACE imGUI_dashBoard_blocks::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashBoard_blocks
)

# Generate and install the package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imGUI_dashBoard_blocksConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imGUI_dashBoard_blocksConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashBoard_blocks
)

# Cpy config
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imGUI_dashBoard_blocksConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imGUI_dashBoard_blocks
)

# Install the web files to share directory
install(DIRECTORY web/ 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dashBoard_blocks/web
)