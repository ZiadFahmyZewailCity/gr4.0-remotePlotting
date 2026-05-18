#include "imgui.h"
// Hooks ImGui up to SDL2 (Window & Input)
#include "backends/imgui_impl_sdl2.h" 
// Hooks ImGui up to OpenGL3 / WebGL (Graphics Rendering)
#include "backends/imgui_impl_opengl3.h"
// The SDL2 library provided by Emscripten
//  SDL is simple directMedia layer, its a libary which connects the c++ code with the actual hardware 
#include <SDL.h> 
// The WebGL graphics library provided by Emscripten
#include <GLES3/gl3.h> 
// Required so we can use emscripten_set_main_loop()
#include <emscripten.h> 
#include <emscripten/websocket.h>

#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include "../json.hpp"

// Global variables for the window infrastructure
SDL_Window* g_Window;
SDL_GLContext g_GLContext;
//Socket is set to global for now so data can be sent from he UI loop
EMSCRIPTEN_WEBSOCKET_T g_WebSocket = 0; 


bool is_connected = false;
bool recieved_config = false;

//Structs for dashboard elements 
enum class dashboardElementType { TIME_SERIES, SLIDER, TEXT_LABEL};

//Structs which would be better for actual development and not POC
//Should probably look into using C++ variants to make structs cleaner
/*
struct plot_struct{
    std::string x_axis_label;
    std::string y_axis_label;
    std::string data_source;
    std::vector<float> databuffer;
};

struct slider_struct{
    float min_val;
    float max_val;
    float current_val;

};

struct text_struct{
    std::string text;
};

struct dashboardElement{
    std::string id;
    std::string title;

    plot_struct plot_object;
    slider_struct slider_object;
    text_struct text_object;
};
*/

//Simplified dashboard element struct
struct dashboardElement {

    std::string id;
    std::string title;
    
    dashboardElementType type;

    std::string data_source;
    std::vector<float> databuffer;
    float current_val = 0.0f;
    
    // NEW: Tracks if the user is holding down the mouse on this widget
    bool is_being_edited = false; 
    int edit_cooldown = 0;
};

//Each window contains a set of widgets
//This is probably going to be very powerful for customization
struct dashboardPanel {
    std::string panel_name;
    std::vector<dashboardElement> dashboardObjects;
};

std::vector<dashboardPanel> current_dashboard;



/*
So my current understanding is that the standard linux libraries
for sockets cant be used here as they are blocking
You cant really have anything blocking in a browser
So you should use the emscripten sockets which are non blocking

Their setup is pretty similar to setting up any sockets

1) You define attributes for your socket
2) Create the websocket using emscripten_websocket_new with the parameters you defined
This returns an ID to the socket created
3) You define some function callbacks, the definitions of these sockets is restricted by the
emscripten. These are the functions that are triggered when some websocket related
event occurs
The definition of the functions is restricted by emscripten as they must return a 
EM_TRUE type, the compiler will through an error if a function is not defined correct and 
returns thsi type

4) You wire up these functions to specific callbacks 
*/



//Step 3 defining the callbacks for emscripten sockets
EM_BOOL callback_succefulConnect(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData){

    is_connected = true;
    std::cout << "Connected to backend" << std::endl;
    return EM_TRUE;
}

EM_BOOL callback_Close(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData){
    is_connected = false;
    recieved_config = false;
    std::cout << "Disconnected from backend" << std::endl;
    return EM_TRUE;
}

EM_BOOL callback_Run(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData){


    //take raw bytes, cast them to char, need to tell 
    //number of bytes as network packets may not have
    //terminator \0
    std::string payload((const char*)websocketEvent->data,websocketEvent->numBytes);
    //parsing the text to get the json file
    nlohmann::json message = nlohmann::json::parse(payload);
    
    //We either recieve a config file or a telemetry file
    if(message["msg_type"] == "config"){

        //Clear previous dashboard if a new config arrives 
        current_dashboard.clear();

        //We iterate through each panel and add it to a panel vector
        for(auto& json_panel : message["panels"]){
            
            dashboardPanel newPanel;
            newPanel.panel_name = json_panel["panel_name"];

            //We iterate through each panel object in a panel and store them
            //as part of the panel 
            for(auto& item : json_panel["dashboardElement"])
            {
                dashboardElement new_dashboardElement;
                new_dashboardElement.id = item["id"];
                new_dashboardElement.title = item["title"];
                new_dashboardElement.data_source = item.value("data_source", "");
            
                if(item["type"] == "timeseries"){
                    new_dashboardElement.type = dashboardElementType::TIME_SERIES;
                    new_dashboardElement.databuffer.resize(100,0.0f);
                }
                else if(item["type"] == "widget")
                {
                    new_dashboardElement.type = dashboardElementType::SLIDER;
                }
                else if(item["type"] == "text")
                {
                    new_dashboardElement.type = dashboardElementType::TEXT_LABEL;
                }
            
                newPanel.dashboardObjects.push_back(new_dashboardElement);
            
            }
            current_dashboard.push_back(newPanel);

        }

        recieved_config = true;
    }

    //In the actual implemention we should use some kind of look up table O(1) and not have to iterate through an entire vecctor O(n)
    else if (message["msg_type"] == "telemetry") {
        
        // Loop through every incoming piece of data in the JSON
        for (auto& [json_key, json_value] : message.items()) {
            // Skip the standard header data
            if (json_key == "msg_type" || json_key == "timestamp") continue;

            // Search our vector for a widget that owns this specific data source
            for (auto& panel : current_dashboard) {
                for(auto& object : panel.dashboardObjects) {
                
                    if (object.data_source == json_key && object.type == dashboardElementType::TIME_SERIES) { 
                        
                        // Found the correct object, erase oldest data point and plot new one
                        object.databuffer.erase(object.databuffer.begin());
                        object.databuffer.push_back(json_value.get<float>());
                        
                        // Match found break out
                        break; 
                    }
                    
                    else if (object.data_source == json_key && object.type == dashboardElementType::SLIDER) {
                        
                        //Dont update the value of the widget if its being edited
                        if (!object.is_being_edited && object.edit_cooldown == 0) {
                            object.current_val = json_value.get<float>();
                        }
                        break;
                    }
                }
            }
        }
    }

    return EM_TRUE;

}

//All function calls needed to create a frame call in the beginning 
void createNewFrame(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void main_loop(){

    // Process input (mouse clicks, dragging)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    createNewFrame();

    // Window for the status of connection
    if(ImGui::Begin("Connecting to server")){
        if(!is_connected){
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
        }
        else{
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
        }
    }
    ImGui::End();

    //This is the code for the actual plotting based on the config file
    if(is_connected && recieved_config){
        for(auto& panel : current_dashboard){
            if(ImGui::Begin(panel.panel_name.c_str()))
            {
                for(auto& object : panel.dashboardObjects){
                    
                    if(object.type == dashboardElementType::TEXT_LABEL){
                        ImGui::Text("%s", object.title.c_str());
                    }
                    else if (object.type == dashboardElementType::TIME_SERIES){
                        ImGui::PlotLines(object.title.c_str(), object.databuffer.data(), object.databuffer.size(), 0, NULL, -2.0f, 2.0f, ImVec2(400, 150));                    
                    }
else if (object.type == dashboardElementType::SLIDER) {   
                        
                        // If the slider is moving at all, send the data immediately!
                        if (ImGui::SliderFloat(object.title.c_str(), &object.current_val, 0.1f, 5.0f)) {
                            nlohmann::json command_msg;
                            command_msg["msg_type"] = "control";
                            command_msg["id"]       = object.id;
                            command_msg["value"]    = object.current_val;
                            
                            std::string payload = command_msg.dump();
                            emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                        }

                        // Protect the slider from incoming Python data while dragging
                        object.is_being_edited = ImGui::IsItemActive();

                        // Reset the cooldown shield every frame the item is held, not just
                        // frames where the value changed. This guarantees the shield is fully
                        // armed at the moment of release, closing the race on the release frame.
                        if (object.is_being_edited) {
                            object.edit_cooldown = 60;
                        }
                        
                        // Once we let go, tick down the shield for 1 second so Python can catch up
                        if (!object.is_being_edited && object.edit_cooldown > 0) {
                            object.edit_cooldown--;
                        }
                    }
                    
                    ImGui::Spacing();
                }
            }
            ImGui::End();
        }
    }

    // Render the graphics
    ImGui::Render();
    glViewport(0, 0, 1280, 720);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_Window);

}

void connect_SDL(){
    // Setup SDL for the browser
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    g_Window = SDL_CreateWindow("ImGui Emscripten Setup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    g_GLContext = SDL_GL_CreateContext(g_Window);
    SDL_GL_MakeCurrent(g_Window, g_GLContext);

}

void imGUI_contextSetup(){
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    //This context is what essentailly tracks everything, its 
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(g_Window, g_GLContext);
    ImGui_ImplOpenGL3_Init("#version 300 es");
}

EMSCRIPTEN_WEBSOCKET_T emscripten_socketSetup(std::string URL, 
    em_websocket_open_callback_func on_open, 
    em_websocket_close_callback_func on_close, 
    em_websocket_message_callback_func on_message){
    
    //SETTING UP EMSCRIPTEN WEBSOCKETS 

    //Step 1
    //Struct made for defining some qualties of the websocket
    EmscriptenWebSocketCreateAttributes webSocket_attributes = {
        //Define the URL you want to connect to
        URL.c_str(), 
        //A parameter for using subprotocols not sure what those, can be left as null'
        //Will keep null for now
        NULL,                  
        //Creates main thread
        EM_TRUE                
    };

    //Step 2
    EMSCRIPTEN_WEBSOCKET_T webSocket = emscripten_websocket_new(&webSocket_attributes);

    //Step 4
    emscripten_websocket_set_onopen_callback(webSocket, NULL, on_open);
    emscripten_websocket_set_onclose_callback(webSocket, NULL, on_close);
    emscripten_websocket_set_onmessage_callback(webSocket, NULL, on_message);

    return webSocket;

}

int main(){

    connect_SDL();

    imGUI_contextSetup();

    // FIX: Assign the return value directly to g_WebSocket instead of a local variable.
    // The local variable `my_socket` would shadow the global, leaving g_WebSocket = 0
    // and causing all emscripten_websocket_send_utf8_text() calls to silently fail.
    g_WebSocket = emscripten_socketSetup(
        "ws://localhost:9000",
        callback_succefulConnect,
        callback_Close,
        callback_Run
    );

    // Tell Emscripten to run your main_loop function forever
    // A standard while loop would cause the browser to be unable to do anything 
    emscripten_set_main_loop(main_loop, 0, true);

    return 0;

}