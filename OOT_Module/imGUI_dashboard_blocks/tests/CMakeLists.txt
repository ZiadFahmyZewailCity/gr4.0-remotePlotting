# 1. Fetch Boost.UT micro-testing framework
include(FetchContent)
FetchContent_Declare(
    boost_ut
    GIT_REPOSITORY https://github.com/boost-ext/ut.git
    GIT_TAG        v2.0.1
)
FetchContent_MakeAvailable(boost_ut)

# 2. Compile QA binary strictly linked to backend implementation
add_executable(qa_dashBoardServer 
    qa_dashBoardServer.cpp
    ../src/dashBoard_server.cpp
)

# 3. Map local headers (Pulls in ../src/json.hpp and ../src/dashBoard_server.hpp)
target_include_directories(qa_dashBoardServer PRIVATE
    ../include
    ../src
)

# 4. Link inherited core drivers
target_link_libraries(qa_dashBoardServer PRIVATE
    Boost::ut
    cppzmq
    Threads::Threads
)

if(TARGET websocketpp::websocketpp)
    target_link_libraries(qa_dashBoardServer PRIVATE websocketpp::websocketpp)
elseif(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    target_include_directories(qa_dashBoardServer PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
endif()

# 5. Register with CTest
add_test(NAME qa_dashBoardServer COMMAND qa_dashBoardServer)