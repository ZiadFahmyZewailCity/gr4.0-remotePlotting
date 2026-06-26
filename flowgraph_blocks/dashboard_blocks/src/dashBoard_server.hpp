#pragma once

#include <cstdint>
#include <mutex>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/frame.hpp>
#include <websocketpp/http/constants.hpp>
#include <websocketpp/logger/levels.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <set>
#include <string>
#include <zmq.h>
#include <zmq.hpp>

//TO DO: IPC Handling using ZMQ

//Defining server
typedef websocketpp::server<websocketpp::config::asio> server;
using websocketpp::connection_hdl;

class DashboardServer {
private: 

    uint16_t server_port;
    server dashBoardServer;

    //Required so that the ZMQ side of the server process can communicate with the 
    //ZMQ handling side
    zmq::context_t* sharedContext;

    //Variable for setting where the path of the dashboard is
    std::string dashBoard_web_root;

    //Creates a set of the connections the server current holds to dashboards
    std::mutex lockingConnections;
    std::set<connection_hdl, std::owner_less<connection_hdl>> dashBoardServer_connections;

    //CallBack upon HTTP GET request
    //Serve the dashboard to the browser which called the IP of the device
    //running the flowgraph
    void on_http(connection_hdl hdl);

    //On websocket disconnect 
    void on_close(connection_hdl hdl);

    //On websocket connect
    void on_open(connection_hdl hdl);

    //Sending message 
    void on_message(connection_hdl,server::message_ptr msg);

    //Helper function for reading config file  
    bool readFile(const std::string& filepath, std::string& out_content);

public:
    //Constructor Initalizes callbacks 
    explicit DashboardServer(std::string web_root_path);

    //Set port being used to communicate with internet
    void set_port(uint16_t port);
    //ZMQ Context shared with thread handling ZMQ communication with the flowgraph
    void set_ZMQ_context(zmq::context_t &context);
    //Starts up the server
    void runServer();
    //Sends data to all the connected dashboards
    void broadcast_data(const std::string& data);
};