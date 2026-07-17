#include "imgui.h"
#include "implot.h"
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
#include <cstring> // Added for std::memcpy
#include "../json.hpp"

// Global variables for the window infrastructure
SDL_Window* g_Window;
SDL_GLContext g_GLContext;
//Socket is set to global for now so data can be sent from he UI loop
EMSCRIPTEN_WEBSOCKET_T g_WebSocket = 0; 


bool is_connected = false;
bool recieved_config = false;

//TO DO: Dont fully understand this 
// Determines the WebSocket host at runtime from whatever address the browser
// actually used to load this page (localhost, a LAN IP, a hostname, etc).
// This means the same compiled .wasm works from any device on the network
// without needing to hardcode/rebuild for a specific IP.
EM_JS(char*, get_websocket_url, (), {
    var host = window.location.hostname;
    var url = "ws://" + host + ":9090";
    var lengthBytes = lengthBytesUTF8(url) + 1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(url, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});


//Structs for dashboard elements 
enum class dashboardElementType { TIME_SERIES, SLIDER, TEXT_LABEL};

//Simplified dashboard element struct
struct dashboardElement {

    std::string id;
    std::string title;
    
    dashboardElementType type;

    std::string data_source;
    std::vector<float> databuffer;
    float current_val = 0.0f;
    
    // Tracks if the user is holding down the mouse on this widget
    bool is_being_edited = false; 
    int edit_cooldown = 0;
};

//Each window contains a set of widgets
struct dashboardPanel {
    std::string panel_name;
    std::vector<dashboardElement> dashboardObjects;
};

std::vector<dashboardPanel> current_dashboard;

void callback_configLoaded(void* arg, void* buffer, int buffer_size) {
    std::string payload(static_cast<const char*>(buffer), buffer_size);
    nlohmann::json message;
    
    try {
        message = nlohmann::json::parse(payload);
    } catch (...) {
        std::cout << "Error parsing incoming JSON config." << std::endl;
        return;
    }

    current_dashboard.clear();

    if (message.contains("panels")) {
        for (auto& json_panel : message["panels"]) {
            dashboardPanel newPanel;
            newPanel.panel_name = json_panel["panel_name"];

            for (auto& item : json_panel["dashboardElement"]) {
                dashboardElement new_dashboardElement;
                new_dashboardElement.id = item["id"];
                new_dashboardElement.title = item["title"];
                new_dashboardElement.data_source = item.value("data_source", "");
            
                if (item["type"] == "timeseries") {
                    new_dashboardElement.type = dashboardElementType::TIME_SERIES;
                    new_dashboardElement.databuffer.resize(100, 0.0f);
                }
                else if (item["type"] == "widget") {
                    new_dashboardElement.type = dashboardElementType::SLIDER;
                }
                else if (item["type"] == "text") {
                    new_dashboardElement.type = dashboardElementType::TEXT_LABEL;
                }
                newPanel.dashboardObjects.push_back(new_dashboardElement);
            }
            current_dashboard.push_back(newPanel);
        }
        recieved_config = true;
        std::cout << "Dashboard UI Built from Backend Config!" << std::endl;
    }
}

void callback_configFailed(void* arg) {
    std::cout << "Failed to fetch /config.json from server." << std::endl;
}

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

EM_BOOL callback_Run(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData) {
    
    // --- BRANCH 1: TEXT DATA (Configuration) ---
    if (websocketEvent->isText) {
        return EM_TRUE;
    } 
    // --- BRANCH 2: BINARY DATA (Live DSP Telemetry) ---
    else {
        if (!recieved_config) return EM_TRUE; // Ignore data until UI is built

        // DIAGNOSTIC: Print every 60th frame to the console so we know data is arriving
        static int frame_counter = 0;
        if (frame_counter++ % 60 == 0) {
            std::cout << "[Network] Binary telemetry received: " << websocketEvent->numBytes << " bytes" << std::endl;
        }

        // Update our plots with the new data
        for (auto& panel : current_dashboard) {
            for (auto& object : panel.dashboardObjects) {
                
                if (object.type == dashboardElementType::TIME_SERIES) { 
                    
                    size_t topic_len = object.id.size();
                    
                    // Match incoming binary packet header against this plot's topic ID
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        
                        size_t data_offset = topic_len;
                        // Advance pointer past common ASCII delimiters (: or \0 or space)
                        if (data_offset < websocketEvent->numBytes && 
                            (((const char*)websocketEvent->data)[data_offset] == ':' || 
                             ((const char*)websocketEvent->data)[data_offset] == '\0' || 
                             ((const char*)websocketEvent->data)[data_offset] == ' ')) {
                            data_offset++;
                        }

                        // WASM FIX: Safely copy bytes to prevent silent unaligned memory crashes
                        int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                        int num_floats = valid_payload_bytes / sizeof(float);
                        
                        if (num_floats > 0) {
                            std::vector<float> incoming_floats(num_floats);
                            std::memcpy(incoming_floats.data(), websocketEvent->data + data_offset, valid_payload_bytes);

                            // Added a buffer for better plotting
                            for (int i = 0; i < num_floats; i++) {
                                object.databuffer.push_back(incoming_floats[i]);
                            }
            
                            int max_window_size = 9600;
                            
                            if (object.databuffer.size() > max_window_size) {
                                // Erase the oldest samples from the front of the vector
                                object.databuffer.erase(
                                    object.databuffer.begin(), 
                                    object.databuffer.begin() + (object.databuffer.size() - max_window_size)
                                );
                            }
                            
                            // Prevent ImGui crash on empty buffer
                            if (object.databuffer.empty()) {
                                object.databuffer.push_back(0.0f);
                            }
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
                        
                        //Check if we actually have data in the buffer
                        if (object.databuffer.size() > 0) {
                            
                            std::string hidden_id = "##" + object.id;
                            
                            if (ImPlot::BeginPlot(object.title.c_str(), ImVec2(-1, 200))) {
                                
                                //Axis labels
                                ImPlot::SetupAxes("Samples", "Amplitude");
                                //Zooming in and out
                                ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0, ImPlotCond_Once);
                                
                                // We keep X locked to the buffer size so it acts like a live oscilloscope screen
                                ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)object.databuffer.size(), ImPlotCond_Always);
                                
                                ImPlot::PlotLine(hidden_id.c_str(), object.databuffer.data(), (int)object.databuffer.size());
                                ImPlot::EndPlot();                        
                            } 
                        } 
                        else {
                            // FIXED BUG: This is now correctly attached to the buffer size check!
                            // warning so we know the UI is working but waiting
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for DSP telemetry stream...");
                        }
                    }
                    else if (object.type == dashboardElementType::SLIDER) { 
                        
                        if (ImGui::SliderFloat(object.title.c_str(), &object.current_val, 0.1f, 100.0f)) {
                            
                            // Package the ID and the value into a JSON object
                            nlohmann::json command_msg;
                            command_msg["target"] = object.id;          
                            command_msg["value"]  = object.current_val; 
                            
                            // Convert to string and send
                            std::string payload = command_msg.dump();
                            emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                        }

                        // these two functions are so the process of dragging slider is smooth and doesnt stutter 
                        object.is_being_edited = ImGui::IsItemActive();
                        if (object.is_being_edited) {
                            object.edit_cooldown = 60;
                        }
                        
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
    //ImPlot Context
    ImPlot::CreateContext();
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

    emscripten_async_wget_data("/config.json", NULL, callback_configLoaded, callback_configFailed);


    //TO DO: Dont fully understand how this works, got it from somewhere on the internet
    // Build the websocket URL from the page's own host instead of a hardcoded
    // IP, so this works whether the page was loaded via localhost or a LAN IP.
    char* ws_url_raw = get_websocket_url();
    std::string ws_url(ws_url_raw);
    free(ws_url_raw);
    std::cout << "Connecting to WebSocket at: " << ws_url << std::endl;


    // Assign the return value directly to g_WebSocket
    g_WebSocket = emscripten_socketSetup(
        ws_url,
        callback_succefulConnect,
        callback_Close,
        callback_Run
    );

    // Tell Emscripten to run your main_loop function forever
    emscripten_set_main_loop(main_loop, 0, true);

    return 0;
}