#include "dashBoard_server.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <websocketpp/common/connection_hdl.hpp>
#include <zmq.hpp>
#include "json.hpp"

//Constructor Initalizes callbacks 
DashboardServer::DashboardServer(std::string web_root_path) 
    : dashBoard_web_root(std::move(web_root_path)) {
    //Initalize asynchrnous communication
    dashBoardServer.init_asio();

    //Set handlers to previously created functions

    //Handler on HTTP GET Request
    dashBoardServer.set_http_handler([this](connection_hdl hdl) {
        on_http(hdl);
    });

    dashBoardServer.set_open_handler([this](connection_hdl hdl) {
        on_open(hdl);
    });

    //Handler on closing of websocket connection
    dashBoardServer.set_close_handler([this](connection_hdl hdl) {
        on_close(hdl);
    });

        dashBoardServer.set_message_handler([this](connection_hdl hdl, server::message_ptr msg) {
        on_message(hdl, msg);
    });

    //Disable verbose websocket messages
    dashBoardServer.clear_access_channels(websocketpp::log::alevel::all);
}

//Set port variable and sets dashboardServer to listen to port
void DashboardServer::set_port(uint16_t port) {
    server_port = port;
    dashBoardServer.listen(server_port);
}

void DashboardServer::runServer() {
    dashBoardServer.start_accept();
    dashBoardServer.run();
}

void DashboardServer::on_http(connection_hdl connection_handle) {

    //The connection handler is a ptr which points to a connection object which holds all the details about the current 
    //http request
    server::connection_ptr con = dashBoardServer.get_con_from_hdl(connection_handle);

    std::string URI = con->get_resource();
    std::cout << "GET Request from: " << URI << "\n";


    std::string target_filename;
    std::string content_type;

    //Use URI to serve the files to the browser
    //Apparently you need to specifiy the content type aswell as the browser wont run something it doesnt know the type of
    if (URI == "/" || URI == "/dashboard.html") {
        target_filename = "dashboard.html";
        content_type = "text/html";
    } else if (URI == "/dashboard.js") {
        target_filename = "dashboard.js";
        content_type = "application/javascript";
    } else if (URI == "/dashboard.wasm") {
        target_filename = "dashboard.wasm";
        content_type = "application/wasm";
    } else if (URI == "/config.json") {
        target_filename = "config.json";
        content_type = "application/json";
    } else {
        con->set_status(websocketpp::http::status_code::not_found);
        return;
    }

    std::string full_path = dashBoard_web_root + target_filename;
    std::string file_content;
    
    //Read File path
    if (readFile(full_path, file_content)) {
        
        con->append_header("Content-Type", content_type);
        //If the read is sucessful set the body of the http reply to be the contents of the file
        con->set_body(file_content);
        //Set the status of reply 
        con->set_status(websocketpp::http::status_code::ok);
        std::cout << "  -> Served: " << target_filename << (" (" + std::to_string(file_content.size()) + " bytes)\n");
    } else {
        std::cerr << "  -> [404 ERROR] File missing on hard drive: " << full_path << "\n";
        con->set_status(websocketpp::http::status_code::not_found);
    }

}

void DashboardServer::on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(lockingConnections);
    dashBoardServer_connections.insert(hdl);
    std::cout << "Client connected\n";
}

void DashboardServer::on_message(connection_hdl hdl, server::message_ptr msg){

    //Check that the context is valid
    if(!sharedContext)
    {
        //TO DO: Remove
        std::cout << "Shared Context not found\n";
        return ;
    }

    std::string ws_payload = msg->get_payload();

    try {
        auto json_cmd = nlohmann::json::parse(ws_payload);
        std::string target_id = json_cmd.value("target", "");
        float val = json_cmd.value("value", 0.0f);
        std::string zmq_frame = target_id + ":" + std::to_string(val);

        //Generate the internal connection, this should function as a deepCopy and so should be super 
        //fast and non-blocking
        zmq::socket_t internal_connection_daemon(*sharedContext, zmq::socket_type::push);
        internal_connection_daemon.set(zmq::sockopt::linger, 0);
        internal_connection_daemon.connect("inproc://commands");

        //Commands from dashBoard
        zmq::message_t commands_to_flowgraph(zmq_frame.begin(), zmq_frame.end());
        

        internal_connection_daemon.send(commands_to_flowgraph, zmq::send_flags::none);
    }
    catch (...)
    {

    }

}

void DashboardServer::set_ZMQ_context(zmq::context_t &context){
    sharedContext = &context;
}


//On Disconnect 
void DashboardServer::on_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(lockingConnections);
    dashBoardServer_connections.erase(hdl);
    std::cout << "Client disconnectd\n";
}

//Send data as raw bytes to all connections to support multiple dashboards being open
void DashboardServer::broadcast_data(const std::string& data) {

    std::lock_guard<std::mutex> lock(lockingConnections);
    //Iterates through the current connections list to the server and sends the data to each one of them 
    for (auto hdl : dashBoardServer_connections){
        dashBoardServer.send(hdl, data, websocketpp::frame::opcode::binary);
    }
}

//TO DO: Figure out how this function works, i kinda just took it from somewhere on the internet
bool  DashboardServer::readFile(const std::string& filepath, std::string& out_content){
   
    //File object
    std::ifstream file(filepath, std::ios::binary);

    //Attempts to read file
    if(!file.is_open()) { return false; }

    //read file content into out_content
    out_content.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return true;



}