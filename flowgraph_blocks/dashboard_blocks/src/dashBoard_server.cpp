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
//The address is set to be resuable
//TO DO: Perhaphs not a good idea to reset the reuse address here, breaks function doing single thing idea
void DashboardServer::set_port(uint16_t port) {
    
    //Set so the address is reusable
    dashBoardServer.set_reuse_addr(true);
    dashBoardServer.listen(port);
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

    std::string SYNC_CMD =  "SERVER:SYNC"; 
    dispatch_internal_message(SYNC_CMD);
}

void DashboardServer::on_message(connection_hdl hdl, server::message_ptr msg){


    std::string ws_payload = msg->get_payload();

    //This does the json parsing and turns the json message from the widget to just normal ascii to be sent to the flowgraph
    //Note this is in the server side of the dashboard server and so needs to be moved within the dashboard server process to
    //the part of the server which handles the IPC with the flowgraph 
    try {

        //Parse the json
        auto json_cmd = nlohmann::json::parse(ws_payload);
        
        //Find the widget
        std::string target_id = json_cmd.value("target", "");
        
        //This is the packet being sent
        std::string zmq_frame = target_id + ":";

        std::cout << "Code has changed " << "\n";

        //Check if whats being rcved is a number or a string. As the textBox has to send a string it must be handled sepeartly as to not throw and error
        if (json_cmd["value"].is_number()) {
            
            //Get value, appened it to the zmq_frame
            float num_val = json_cmd["value"].get<float>();
            zmq_frame += std::to_string(num_val);
        } 
        else if (json_cmd["value"].is_string()) {
            
            //Get the string, appened it to the zmq_frame
            std::string str_val = json_cmd["value"].get<std::string>();
            zmq_frame += str_val;
        } 
        else if (json_cmd["value"].is_boolean()) {

            bool bool_val = json_cmd["value"].get<bool>();
            zmq_frame += (bool_val ? "1" : "0");

        }


        dispatch_internal_message(zmq_frame);
    }
    //Error messages made by AI, not sure what would be a good error message so thought AI would give good pointers.
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[JSON Error] Malformed WebSocket payload: " << e.what() << "\n";
    }
    //TO DO: I think there is a double error message here because the try/catch of the dispatch_internal_message will also send out a message
    //Fix later
    catch (const std::exception& e) {
        std::cerr << "[Server Error] Unexpected error in on_message: " << e.what() << "\n";
    }
}

void DashboardServer::set_ZMQ_context(zmq::context_t &context){
    sharedContext = &context;

    //Trying to fix an issue with SYNC message not firing when connecting to the dashboard instance
    internal_PUSH_SOCKET = std::make_unique<zmq::socket_t>(*sharedContext, zmq::socket_type::push);
    internal_PUSH_SOCKET->connect("inproc://commands");
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


void DashboardServer::dispatch_internal_message(const std::string& cmd){

    
    //TO DO: Remove debug message
    std::cout << "internal Messaging function called" << "\n";

    //Check that the context is valid
    if(!sharedContext)
    {
        //TO DO: Remove debug message
        std::cout << "Shared Context not found\n";
        return ;
    }

    //TO DO: Remove debug message
    if (!internal_PUSH_SOCKET) {
        
        std::cout << "Socket not intialized";
        return;
    }

    try {
        
        //Commands from dashBoard
        zmq::message_t commands_to_flowgraph(cmd.begin(), cmd.end());
        internal_PUSH_SOCKET->send(commands_to_flowgraph, zmq::send_flags::none);
        //TO DO: Remove
        std::cout << "Push should be complete" << "\n";

    }
    catch(...){}


}