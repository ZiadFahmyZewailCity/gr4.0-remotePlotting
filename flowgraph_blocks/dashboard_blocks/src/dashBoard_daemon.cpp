#include "dashBoard_server.hpp"
#include <zmq.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <thread>

#include <chrono>


#include <fstream>
#include <iostream>


#include "json.hpp"



void read_config_file(zmq::socket_t & flowgraphSocket, const std::string &config_file_path){

    std::ifstream config_file(config_file_path);
    if (!config_file.is_open()) {
        return;
    }


    try{
            
    nlohmann::json config_file_data = nlohmann::json::parse(config_file);

    for(const auto& panel: config_file_data["panels"])
    {
        for(const auto& element : panel["dashboardElement"]){
            std::string id = element.value("id","");
            std::string type = element.value("type","");


            //This will be a check for all the types of plots we support
            if(type == "timeseries"){

                //This makes it so the ZMQ subscriber of the flowgraph only 
                //accepts messages with these ids
                flowgraphSocket.set(zmq::sockopt::subscribe, id);

            }
            else if (type == "widget")
            {
                //Nothing actually needs to be done here probably, keep the if statement in case future dictates a use case
            }



        }
    }

        return;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return ;
    }










}

//This should just run the dashboard
int main(){
    
    //Pass path to the dashboard files
    std::string web_root = "../web/";
    DashboardServer dashBoard_server(web_root);
    //Pass path to the config file
    std::string config_path = web_root + "config.json";
    
    //Hardcoded for now should be changed to be configured by the flowgraph
    dashBoard_server.set_port(9090);

    //TO DO: Dont fully understand this syntax in particular for starting up a thread 
    std::thread serverThread([&dashBoard_server]()
    {
        dashBoard_server.runServer();
    });


    /*
        Data from multiple blocks must be aggregated, this means 
        we either open IPC for each block or somehow put all the data into one socket
        We can do this via ZMQs poller, which is not actually polling based but interrupt based 
    */


    //Define the context for the ZMQ
    zmq::context_t context;
    //Share the state for the zmq betweent the processes
    dashBoard_server.set_ZMQ_context(context);

    
    //Socket definitons
    //Reciving data from GR4 Flowgraph
    zmq::socket_t data_aggregation(context, zmq::socket_type::sub);
    //Sending commands coming down from the dashboard to the flowgraph
    zmq::socket_t commands_toFlowGraph(context, zmq::socket_type::pub);
    //Sending commands from the background websocket thread
    zmq::socket_t internal_cmd_rx(context, zmq::socket_type::pull);

    //Reading config file in order to only accept message with the correct header ID
    //May not be that necessary
    read_config_file(data_aggregation ,config_path);

    data_aggregation.set(zmq::sockopt::subscribe, "");
    data_aggregation.bind("tcp://*:5555");

    //Socket connections
    //TO DO: Should find a way to automate this to avoid port conflict 
    commands_toFlowGraph.bind("tcp://*:5556");
    
    //TO DO: Figure this out
    internal_cmd_rx.bind("inproc://commands");

    //This array of sockets are handled by the poll function
    //Note revents is the member variable which stores if the socket has recieved a message or not
    zmq::pollitem_t sockets[] = 
    {
        { static_cast<void*>(data_aggregation),0,ZMQ_POLLIN,0},
        { static_cast<void*>(internal_cmd_rx),0,ZMQ_POLLIN,0}
    };

    while(true){

        // This function is called poll but its actually a non blocking and interrupt based
        // You give it your sockets and the timeout time you want for the process to be slept for
        // -1 means this thread will remain asleep until a network packet associated with a port that one of the 
        // sockets the thread is listening 
        zmq::poll(&sockets[0],2, std::chrono::milliseconds(-1));

        //Data coming from flowgraph to dashboard/s
        if(sockets[0].revents & ZMQ_POLLIN){

            zmq::message_t data_sinks;
            auto recived_sinks = data_aggregation.recv(data_sinks,zmq::recv_flags::none);
            
            //Look into sending a certain verison of recived sinks message
            if(recived_sinks) { 
                dashBoard_server.broadcast_data(data_sinks.to_string());
            };

        }

        //Commands coming from the dashBoard to flowgraph
        if(sockets[1].revents & ZMQ_POLLIN){

            zmq::message_t dashBoard_command_message;
            //Flag need to be added apparently or a deprecated version of the function is used, 
            // should not be an issue, 
            // the only differce between the two just seems to be that the newer version is strictly typed when it comes to flags
            auto recived_command = internal_cmd_rx.recv(dashBoard_command_message, zmq::recv_flags::none);
            if(recived_command){
                //Look into version to see if we need a 
                commands_toFlowGraph.send(dashBoard_command_message, zmq::send_flags::none);
            }
        }

    }



    //The thread never techincally ends, but i do think if one of the threads gets a kill order (SIGINT)
    //This will make it so the other closes without issue ?
    serverThread.join();








}