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
#include <algorithm>
#include <cstddef>
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
enum class dashboardElementType { TIME_SERIES, VECTOR_SINK , FREQUENCY_SINK, CONSTELLATION_SINK, WATERFALL_SINK , SLIDER, TEXTLABEL, BUTTON, CHECKBOX, DROPDOWN, TEXTBOX, UNKNOWN };

//Simplified dashboard element struct
struct dashboardElement {

    std::string id;
    std::string title = "default";
    
    dashboardElementType type;

    //Data sources 
    std::string data_source;
    std::vector<float> databuffer;
    float current_val = 0.0f;
    bool  current_val_bool = false;

    std::string x_axis_label  = "default_x_axis";
    std::string y_axis_label  = "default_y_axis";


    //This flag is meant to prevent plotting of zeros if no data is being sent to the sink
    //Instead it will either display the last frame or if no data has been sent, it'll display a waiting for data message
    bool has_rcved_data = false;
    
    // Axis boundries
    int x_axis_min = 0;
    int x_axis_max = 100;
    int y_axis_min = 0;
    int y_axis_max = 100;


    //TO DO: These are dashboard element specific, 
    //it may be a good idea to make this a bit cleaner in the future, its very little overhead 
    

    // Tracks if the user is holding down the mouse on this widget
    bool is_being_edited = false; 
    int edit_cooldown = 0;

    //DropDown widget options
    std::vector<std::string> options;
    int current_val_int = 0;


    // FREQUENCY/WATERFALL SINK PARAMS
    int windowSize = 1024;
    float sample_rate = 1.0f;
    double start_freq = 0.0;
    double step_freq = 1.0;

    //Waterfall only
    int history_size = 100;

    //String Data for textLabel and textBox
    std::string string_current_text = " ";
    char text_buffer[512] = {0};

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
        // Parse the raw payload
        message = nlohmann::json::parse(payload);

        current_dashboard.clear();

        if (message.contains("panels")) {
            for (auto& json_panel : message["panels"]) {
                dashboardPanel newPanel;
                // Safely get the panel name, fallback to "Unnamed Panel" if missing
                newPanel.panel_name = json_panel.value("panel_name", "Unnamed Panel");

                //Intalize
                for (auto& item : json_panel["dashboardElement"]) {
                    dashboardElement new_dashboardElement;
                    
                    //Extract ID, give a fallback if id not found
                    new_dashboardElement.id = item.value("id", "default_id");
                    std::cout << new_dashboardElement.id << "\n";
                    //Extract title, give fallback if title not found
                    new_dashboardElement.title = item.value("title", "default_title");
                    //Extract data source, give fallback if data_source not found
                    new_dashboardElement.data_source = item.value("data_source", "");
                
                    //Sinks
                    if (item["type"] == "timeSeries") {


                        //Read what the x & y axis labels are
                        new_dashboardElement.x_axis_label = item.value("x_axis_label","default_x_axis");
                        new_dashboardElement.y_axis_label = item.value("y_axis_label","default_x_axis");

                        
                        new_dashboardElement.type = dashboardElementType::TIME_SERIES;

                        //TO DO: Buffer size should be set by user
                        new_dashboardElement.databuffer.resize(100, 0.0f);
                    }
                    else if (item["type"] == "frequencySink"){

                        new_dashboardElement.type = dashboardElementType::FREQUENCY_SINK;

                        //Read what the x & y axis labels are
                        new_dashboardElement.x_axis_label = item.value("x_axis_label","default_x_axis");
                        new_dashboardElement.y_axis_label = item.value("y_axis_label","default_x_axis");
                        
                       
                        //Allocating the memory
                        try{
                            new_dashboardElement.windowSize = std::stoi(item.value("windowSize", "1024"));
                            new_dashboardElement.sample_rate = std::stof(item.value("samplingFreq", "1.0"));

                            //The start and end of the frequency graph
                            new_dashboardElement.start_freq = 0;
                            new_dashboardElement.step_freq  = (double)new_dashboardElement.sample_rate / (double)new_dashboardElement.windowSize;

                        } catch (...){
                            //TO DO: debugging message
                            std::cout << "Failed to parse window size on intialization";
                        }
                        new_dashboardElement.databuffer.resize(new_dashboardElement.windowSize, 0.0f);


                    }
                    else if (item["type"] == "waterFallSink"){
                        
                        //Read what the x & y axis labels are
                        new_dashboardElement.x_axis_label = item.value("x_axis_label","default_x_axis");
                        new_dashboardElement.y_axis_label = item.value("y_axis_label","default_x_axis");
                        new_dashboardElement.type = dashboardElementType::WATERFALL_SINK;
                       
                        //Allocating the memory
                        try{

                            new_dashboardElement.windowSize = std::stoi(item.value("windowSize", "1024"));
                            new_dashboardElement.history_size = std::stoi(item.value("historySize","100"));
                            new_dashboardElement.sample_rate = std::stof(item.value("samplingFreq", "1.0"));

                            //The start and end of the frequency graph
                            new_dashboardElement.start_freq = 0;
                            new_dashboardElement.step_freq  = (double)new_dashboardElement.sample_rate / (double)new_dashboardElement.windowSize;

                        } catch (...){
                            //TO DO: debugging message
                            std::cout << "Failed to parse window size on intialization";
                        }

                        new_dashboardElement.databuffer.resize(new_dashboardElement.windowSize * new_dashboardElement.history_size, -140.0f);

                    }
                    else if (item["type"] == "vectorSink"){


                        //Read what the x & y axis labels are
                        new_dashboardElement.x_axis_label = item.value("x_axis_label","default_x_axis");
                        new_dashboardElement.y_axis_label = item.value("y_axis_label","default_x_axis");
                        new_dashboardElement.type = dashboardElementType::VECTOR_SINK;
                        try {
                            new_dashboardElement.windowSize = std::stoi(item.value("vectorSize", "256"));
                        } catch (...){
                            new_dashboardElement.windowSize = 256;
                        }
                        new_dashboardElement.databuffer.resize(new_dashboardElement.windowSize, 0.0f);
                    }
                    else if (item["type"] == "constellationSink"){
                                       
                        //Read what the x & y axis labels are
                        new_dashboardElement.x_axis_label = item.value("x_axis_label","default_x_axis");
                        new_dashboardElement.y_axis_label = item.value("y_axis_label","default_x_axis");
                        new_dashboardElement.type = dashboardElementType::CONSTELLATION_SINK;
                        
                        //Read Buffer size
                        try {
                            //Parse number of points 
                            new_dashboardElement.windowSize = std::stoi(item.value("numberOfPoints", "256"));

                        }   catch (...){
                            new_dashboardElement.windowSize = 256;
                        }

                        //Intialize buffer
                        new_dashboardElement.databuffer.resize(new_dashboardElement.windowSize * 2, 0.0f);
                    }
                    //Widgets
                    else if (item["type"] == "slider") {
                        new_dashboardElement.type = dashboardElementType::SLIDER;
                    }
                    else if (item["type"] == "button") {
                        new_dashboardElement.type = dashboardElementType::BUTTON;
                    }
                    else if (item["type"] == "checkBox") {
                        new_dashboardElement.type = dashboardElementType::CHECKBOX;
                    }
                    else if (item["type"] == "dropdown"){
                        new_dashboardElement.type = dashboardElementType::DROPDOWN;

                        //Attach list of options to the dropDown menu dashBoard element
                        //To do, should create a fall back list
                        if(item.contains("options")){
                            //Iterate for the json objects in option
                            for(auto& option : item["options"]){
                                //Convert to a string
                                std::string option_str = option.get<std::string>();
                                new_dashboardElement.options.push_back(option_str);
                                //TO DO: for debugging remove later
                                std::cout << option_str << "\n";
                            }
                        }
                    }
                    else if (item["type"] == "textBox"){
                        new_dashboardElement.type = dashboardElementType::TEXTBOX;
                    }
                    else if (item["type"] == "textLabel") {
                        new_dashboardElement.type = dashboardElementType::TEXTLABEL;
                    }
                    else 
                    {
                        //This can only exist if the config file is somehow malformed so one of the types either
                        //Had an incorrectly written type
                        //Did not have a type
                        new_dashboardElement.type = dashboardElementType::UNKNOWN;

                        //Debug Message
                        std::cout << "There is a unknown element with id: " << new_dashboardElement.id << "\n";
                    }

                    newPanel.dashboardObjects.push_back(new_dashboardElement);
                }
                current_dashboard.push_back(newPanel);
            }
            recieved_config = true;
            std::cout << "Dashboard UI Built from Backend Config!" << std::endl;
        }
    } catch (const std::exception& e) {
        // Now catches ANY parsing or key-extraction errors without crashing WASM
        std::cout << "Error parsing incoming JSON config: " << e.what() << std::endl;
        return;
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
    
    //Recived Text  
    if (websocketEvent->isText) {
        return EM_TRUE;
    } 
    //Binary Data
    else {
        //Any Data is ignored until the config file is recieved
        if (!recieved_config) return EM_TRUE; 

        //TO DO:Remove later this is for Debugging 
        static int frame_counter = 0;
        if (frame_counter++ % 60 == 0) {
            std::cout << "[Network] Binary telemetry received: " << websocketEvent->numBytes << " bytes" << std::endl;
        }

        // Update our plots with the new data
        for (auto& panel : current_dashboard) {
            for (auto& object : panel.dashboardObjects) {
                
                if (object.type == dashboardElementType::TIME_SERIES 
                    || object.type == dashboardElementType::FREQUENCY_SINK 
                    || object.type == dashboardElementType::VECTOR_SINK
                    || object.type == dashboardElementType::CONSTELLATION_SINK
                    || object.type == dashboardElementType::WATERFALL_SINK) { 
                    
                    size_t topic_len = object.id.size();
                    // Match incoming binary packet header against this plot's topic ID
                    // Checks if buffer has more bytes than the length of the id
                    // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        
                        //Dashboard element has rcved data, no longer want it to display its awaiting data
                        if (!object.has_rcved_data){
                            
                            //Dashboard element has rcved data, no longer want it to display its awaiting data
                            object.has_rcved_data = true;

                        }

                        size_t data_offset = topic_len;
                        // Move data offset past the delimter 
                        // TO DO: Consider using reinterpret_cast instead of c-style cast
                        if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                            data_offset++;
                        
                        
                        //Calculate the length of bytes the data is stored in
                        int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                        //Calculate the number of floats
                        int num_floats = valid_payload_bytes / sizeof(float);
                        
                        //Copy the floats into a vector 
                        if (num_floats > 0) {
                            std::vector<float> incoming_floats(num_floats);

                            //TO DO: Check if this could cause issues somehow
                            std::memcpy(incoming_floats.data(), websocketEvent->data + data_offset, valid_payload_bytes);

                            if(object.type == dashboardElementType::TIME_SERIES){                    
                                //Push floats to buffer 
                                for (int i = 0; i < num_floats; i++) {
                                    object.databuffer.push_back(incoming_floats[i]);
                                }
                
                                //Overwriting the oldest samples with newer samples 
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
                            //Frequency spectrum & Vector sink isnt a rolling buffer. Just replace previous buffer
                            else if(object.type == dashboardElementType::VECTOR_SINK 
                                || object.type == dashboardElementType::CONSTELLATION_SINK){

                                //TO DO: Remove this is for debugging
                                static int dbg_counter = 0;
                                if (dbg_counter++ % 60 == 0) {
                                    std::cout << "[VECTOR BASED MATCH] id='" << object.id
                                            << "' num_floats=" << num_floats
                                            << " first8: ";
                                    for (int i = 0; i < std::min(num_floats, 8); i++)
                                        std::cout << incoming_floats[i] << " ";
                                    std::cout << std::endl;
                                }

                                //Check if the incoming_floats size is smaller than that data buffer
                                std::size_t vectorSize = std::min(incoming_floats.size(), object.databuffer.size());

                                //Copy into the buffer the floats 
                                std::copy(incoming_floats.begin(), incoming_floats.begin() + vectorSize, object.databuffer.begin());                                
                                

                            }
                            else if(object.type == dashboardElementType::FREQUENCY_SINK){     
                            
                                //TO DO: Remove this is for debugging
                                static int dbg_counter = 0;
                                if (dbg_counter++ % 60 == 0) {
                                    std::cout << "[VECTOR BASED MATCH] id='" << object.id
                                            << "' num_floats=" << num_floats
                                            << " first8: ";
                                    for (int i = 0; i < std::min(num_floats, 8); i++)
                                        std::cout << incoming_floats[i] << " ";
                                    std::cout << std::endl;
                                }

                                //Check if the incoming_floats size is smaller than that data buffer
                                std::size_t vectorSize = std::min(incoming_floats.size(), object.databuffer.size());

                                //Copy into the buffer the floats 
                                std::copy(incoming_floats.begin(), incoming_floats.begin() + vectorSize, object.databuffer.begin());   
                                //Zero out the rest of the buffer
                                if (vectorSize < object.databuffer.size()) {
                                    //TO DO, UPDATE THIS TO BE THE DYNAMIC NOISE FLOOR
                                    std::fill(object.databuffer.begin() + vectorSize, object.databuffer.end(), -140.0);
                                }                             
                                
                            }
                            else if(object.type == dashboardElementType::WATERFALL_SINK){

                                //FFT Window size
                                int fft_size = object.windowSize;
                                //size of the history that can be stored
                                int history = object.history_size;

                                //Shift the history down one row
                                std::memmove(object.databuffer.data() + fft_size, //Select everything after that first row
                                            object.databuffer.data(), //Select all the data in the buffer
                                            (history - 1) * fft_size * sizeof(float)); //all the slots in history - 1, times the number of elements per row, times the size of their type. Total memory size minus the first row


                                //If there is a mismatch between the amount that should be plotted according to the window size and whats being sent
                                //This happens for example when a real valued signal is passed through the FFT block, it outputs only half the samples 
                                
                                //Check if there actually is a difference
                                int valid_bins = std::min(num_floats, fft_size);
                                std::memcpy(object.databuffer.data(), incoming_floats.data(), valid_bins * sizeof(float));

                                // Fill the remainder of the row (buffer) with the noise floor
                                if (valid_bins < fft_size) {
                                    std::fill(object.databuffer.begin() + valid_bins, 
                                            object.databuffer.begin() + fft_size, 
                                            -140.0f);
                                }

                            }

                        }
                        break; 
                    }


                }          
                else if (object.type == dashboardElementType::SLIDER) {

                    size_t topic_len = object.id.size();
                    
                    // Match incoming binary packet header against this plot's topic ID
                    // Checks if buffer has more bytes than the length of the id
                    // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        

                        size_t data_offset = topic_len;
                        // Move data offset past the delimter 
                        // TO DO: Consider using reinterpret_cast instead of c-style cast
                        if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                            data_offset++;
                        
                            //Check length to see if its valid
                            int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                            if(valid_payload_bytes >= sizeof(float)){

                                float slider_update = 0.0;
                                //Copy new value into slider update
                                memcpy(&slider_update, websocketEvent->data + data_offset ,sizeof(float));

                                //TO DO: Remove this is for debugging
                                std::cout << "[UI LOGIC] Matched Slider: '" << object.id 
                                        << "' | Value parsed: " << slider_update 
                                        << " | is_being_edited: " << (object.is_being_edited ? "TRUE (Blocked)" : "FALSE (Updating)") 
                                        << std::endl;


                                if (!object.is_being_edited && object.edit_cooldown == 0) {
                                    object.current_val = slider_update;
                                }

                            }

                        break;


                    }
                
                }
                else if (object.type == dashboardElementType::BUTTON){
                    
                    size_t topic_len = object.id.size();
                    // Match incoming binary packet header against this plot's topic ID
                    // Checks if buffer has more bytes than the length of the id
                    // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        

                        size_t data_offset = topic_len;
                        // Move data offset past the delimter 
                        // TO DO: Consider using reinterpret_cast instead of c-style cast
                        if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                            data_offset++;
                        
                            //Check length to see if its valid
                            int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                            if(valid_payload_bytes >= sizeof(bool)){

                                bool button_update = false;
                                //Copy new value into slider update
                                memcpy(&button_update, websocketEvent->data + data_offset ,sizeof(bool));
                                
                                //Button has no persisted state, this is just notification to say that another dashboard 
                                //instance has pressed the button
                                std::cout << "[UI LOGIC] Button pressed elsewhere: '" << object.id << "'" << std::endl;

                            }

                        break;


                    }
                }
                else if (object.type == dashboardElementType::CHECKBOX){

                    size_t topic_len = object.id.size();
                    // Match incoming binary packet header against this plot's topic ID
                    // Checks if buffer has more bytes than the length of the id
                    // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        

                        size_t data_offset = topic_len;
                        // Move data offset past the delimter 
                        // TO DO: Consider using reinterpret_cast instead of c-style cast
                        if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                            data_offset++;
                        
                            //Check length to see if its valid
                            int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                            if(valid_payload_bytes >= sizeof(bool)){
                                        
                                bool checkBox_update = false;
                                //Copy new value into checkbox update
                                memcpy(&checkBox_update, websocketEvent->data + data_offset ,sizeof(bool));

                                //TO DO: Remove this is for debugging
                                std::cout << "[UI LOGIC] Matched CheckBox: '" << object.id 
                                        << "' | Value parsed: " << checkBox_update 
                                        << " | is_being_edited: " << (object.is_being_edited ? "TRUE (Blocked)" : "FALSE (Updating)") 
                                        << std::endl;


                                if (!object.is_being_edited && object.edit_cooldown == 0) {
                                    object.current_val_bool = checkBox_update;
                                }

                            }

                        break;


                    }

                    
                }
                else if (object.type == dashboardElementType::DROPDOWN){

                size_t topic_len = object.id.size();
                // Match incoming binary packet header against this plot's topic ID
                // Checks if buffer has more bytes than the length of the id
                // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                if (websocketEvent->numBytes > topic_len && 
                    std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                    

                    size_t data_offset = topic_len;
                    // Move data offset past the delimter 
                    // TO DO: Consider using reinterpret_cast instead of c-style cast
                    if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                        data_offset++;
                    
                        //Check length to see if its valid
                        int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                        if(valid_payload_bytes >= sizeof(int)){

                            int dropdown_update = 0;
                            //Copy new selected index into dropdown update
                            memcpy(&dropdown_update, websocketEvent->data + data_offset ,sizeof(int));

                            //TO DO: Remove this is for debugging
                            std::cout << "[UI LOGIC] Matched Dropdown: '" << object.id 
                                    << "' | Index parsed: " << dropdown_update 
                                    << " | is_being_edited: " << (object.is_being_edited ? "TRUE (Blocked)" : "FALSE (Updating)") 
                                    << std::endl;

                            if (!object.is_being_edited && object.edit_cooldown == 0 &&
                                dropdown_update >= 0 && dropdown_update < (int)object.options.size()) {
                                object.current_val_int = dropdown_update;
                            }

                    }
                    
                }
                }
                else if (object.type == dashboardElementType::TEXTLABEL){
                    
                    size_t topic_len = object.id.size();
                    // Match incoming binary packet header against this plot's topic ID
                    // Checks if buffer has more bytes than the length of the id
                    // Then compare the first N bytes (Length of the ID) of the packet with the ID of the plot 
                    if (websocketEvent->numBytes > topic_len && 
                        std::strncmp((const char*)websocketEvent->data, object.id.c_str(), topic_len) == 0) {
                        

                        size_t data_offset = topic_len;
                        // Move data offset past the delimter 
                        // TO DO: Consider using reinterpret_cast instead of c-style cast
                        if (data_offset < websocketEvent->numBytes && (((const char*)websocketEvent->data)[data_offset] == ':'))
                            data_offset++;

                        //Text is variable length, not a fixed sizeof(T) like the numeric widgets,
                        //so this just takes whatever bytes are left as the raw text
                        int valid_payload_bytes = websocketEvent->numBytes - data_offset;
                        if (valid_payload_bytes > 0) {
                            object.string_current_text = std::string((const char*)websocketEvent->data + data_offset, valid_payload_bytes);
                        }

                        break;
                    }
                }
            }
        }
        return EM_TRUE;
    }

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
                    
                        //Sinks
                        if (object.type == dashboardElementType::TIME_SERIES){
                            
                            //Check if we actually have data in the buffer
                            if (object.has_rcved_data) {
                                
                                std::string hidden_id = "##" + object.id;
                                
                                if (ImPlot::BeginPlot(object.title.c_str(), ImVec2(-1, 200))) {
                                    
                                    //Axis labels
                                    ImPlot::SetupAxes(object.x_axis_label.c_str(),object.y_axis_label.c_str());
                                    
                                    //TO DO: Should probably just remove this
                                    //Zooming in and out
                                    ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0, ImPlotCond_Once);
                                    
                                    // We keep X locked to the buffer size so it acts like a live oscilloscope screen
                                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)object.databuffer.size(), ImPlotCond_Once);
                                    
                                    ImPlot::PlotLine(hidden_id.c_str(), object.databuffer.data(), (int)object.databuffer.size());
                                    ImPlot::EndPlot();                        
                                } 
                            } 
                            else {

                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for DSP telemetry stream...");
                            }
                        }
                        else if (object.type == dashboardElementType::FREQUENCY_SINK){
                            
                            if(object.has_rcved_data){
                                std::string hidden_id = "##" + object.id;

                                if(ImPlot::BeginPlot(object.title.c_str(), ImVec2(-1, 300))){

                                    ImPlot::SetupAxes(object.x_axis_label.c_str(),object.y_axis_label.c_str());

                                    
                                    double freq_span = (double)object.databuffer.size() * object.step_freq;
                                    ImPlot::SetupAxisLimits(ImAxis_X1, object.start_freq, object.start_freq + freq_span, ImPlotCond_Always);
                                    ImPlot::SetupAxisLimits(ImAxis_Y1, -140.0, 20.0, ImPlotCond_Once);
                                    
                                    //Create plotline
                                    ImPlot::PlotLine(hidden_id.c_str(), object.databuffer.data(), (int)object.databuffer.size(), object.step_freq, object.start_freq);
                                    ImPlot::EndPlot();
                                }

                            }
                            else{
                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for FFT telemetry stream...");
                            }
                        }
                        else if (object.type == dashboardElementType::VECTOR_SINK){
                            
                            if(object.has_rcved_data){
                                std::string hidden_id = "##" + object.id;
                            
                                if(ImPlot::BeginPlot(object.title.c_str(),ImVec2(-1,250))) {
                                    
                                    ImPlot::SetupAxes(object.x_axis_label.c_str(),object.y_axis_label.c_str());
                                    // Lock X-axis bounds to match the vector length
                                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)object.databuffer.size(), ImPlotCond_Always);
                                    ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0, ImPlotCond_Once);

                                    // Render the vector values against indicies
                                    ImPlot::PlotLine(hidden_id.c_str(), object.databuffer.data(), (int)object.databuffer.size());
                                    ImPlot::EndPlot();
                                } 
                                
                            }
                            else {
                                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for vector telemetry stream...");
                            }
                            
                        }
                        else if (object.type == dashboardElementType::CONSTELLATION_SINK) {
                            
                            if (object.has_rcved_data) {
                                std::string hidden_id = "##" + object.id;
                            
                                // A square aspect ratio is usually best for constellation diagrams
                                if (ImPlot::BeginPlot(object.title.c_str(), ImVec2(250, 250)))    {
                                    ImPlot::SetupAxes(object.x_axis_label.c_str(),object.y_axis_label.c_str());

                                    // Apply the user-set boundaries from the JSON config
                                    ImPlot::SetupAxisLimits(ImAxis_X1, -100, 100, ImPlotCond_Once);
                                    ImPlot::SetupAxisLimits(ImAxis_Y1, -100, 100, ImPlotCond_Once);

                                    // Split the interleaved I/Q buffer into separate X and Y arrays
                                    // Made them static to avoid reallocation everytime
                                    static std::vector<float> xs;
                                    static std::vector<float> ys;

                                    xs.resize(object.windowSize);
                                    ys.resize(object.windowSize);

                                    for(int i = 0; i < object.windowSize; i++) {
                                        xs[i] = object.databuffer[i * 2];       // Even indices = I
                                        ys[i] = object.databuffer[i * 2 + 1];   // Odd indices  = Q
                                    }

                                    // Render scatter plot using the separated arrays (4 arguments)
                                    ImPlot::PlotScatter(hidden_id.c_str(), xs.data(), ys.data(), (int)object.windowSize);
                                    ImPlot::EndPlot();
                                } 
                            }
                            else {
                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for constellation telemetry...");
                            }
                        }
                        else if (object.type == dashboardElementType::WATERFALL_SINK){

                            if (object.has_rcved_data) {
                                std::string hidden_id = "##" + object.id;

                                // Apply a colormap (This is the look that looked most like waterfall plots i usually see)
                                ImPlot::PushColormap(ImPlotColormap_Viridis);

                                if (ImPlot::BeginPlot(object.title.c_str(), ImVec2(-1, 300))) {
                                    
                                    // X-axis: Frequency limits. Y-axis: Time (0 to history size)
                                    ImPlot::SetupAxes(object.x_axis_label.c_str(),object.y_axis_label.c_str());

                                    
                                    // Bounds mapping for the corners of the heatmap image
                                    ImPlotPoint bounds_min(object.start_freq, object.history_size);
                                    ImPlotPoint bounds_max(object.start_freq + (object.windowSize * object.step_freq), 0);

                                    // Render the flattened vector as a Heatmap
                                    ImPlot::PlotHeatmap(hidden_id.c_str(), 
                                                        object.databuffer.data(), 
                                                        object.history_size,    // rows
                                                        object.windowSize,      // cols
                                                        -140.0, 0.0,            // Scale min/max bounds (dB limits for the colors)
                                                        nullptr,                // Format string (nullptr for no text overlay)
                                                        bounds_min, bounds_max);
                                    ImPlot::EndPlot();
                                }
                                ImPlot::PopColormap();
                            }



                        }
                        //Widgets
                        else if (object.type == dashboardElementType::SLIDER) { 
                            

                            //To write the title of the widget (Above the widget)
                            ImGui::Text("%s", object.title.c_str());

                            ImGui::SetNextItemWidth(-1.0f);
       
                            std::string hidden_id = "##" + object.id;                     
                            if (ImGui::SliderFloat(hidden_id.c_str(), &object.current_val, 0.1f, 100.0f)) {
                                
                                // Package the ID of the widget and the value into a JSON object
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
                        else if (object.type == dashboardElementType::BUTTON) {
                            

                            //Title will be rendered inside the button
                            std::string hidden_id = object.title + "##" + object.id;
                            //No checkbox specific to implot so we use standard ImGui here
                            if(ImGui::Button(hidden_id.c_str() , ImVec2(-1.0f, 0.0f))){
                                
                                // Package the ID of the widget and the value into a JSON object
                                nlohmann::json command_msg;
                                command_msg["target"] = object.id;          
                                command_msg["value"]  = true; 
                                
                                // Convert to string and send
                                std::string payload = command_msg.dump();
                                emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                                
                            }

                        }
                        else if (object.type == dashboardElementType::CHECKBOX){
                            
                            //To write the title of the widget (Above the widget)
                            ImGui::Text("%s", object.title.c_str());
                            
                            std::string hidden_id = "##" + object.id;
                            ImGui::SetNextItemWidth(-1.0f);
                            //No checkbox specific to implot so just use the one which comes from ImGui
                            if(ImGui::Checkbox(hidden_id.c_str(), &object.current_val_bool)){
                            
                                // Package the ID of the widget and the value into a JSON object
                                nlohmann::json command_msg;
                                command_msg["target"] = object.id;          
                                command_msg["value"]  = object.current_val_bool; 
                                
                                // Convert to string and send
                                std::string payload = command_msg.dump();
                                emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                            }
    
                            //Debouncing
                            object.is_being_edited = ImGui::IsItemActive();
                            if (object.is_being_edited) {
                                object.edit_cooldown = 60;
                            }
    
                            if (!object.is_being_edited && object.edit_cooldown > 0) {
                                object.edit_cooldown--;
                            }   

                            
                        }
                        else if (object.type == dashboardElementType::DROPDOWN){


                            //To write the title of the widget (Above the widget)
                            ImGui::Text("%s", object.title.c_str());
                            
                            std::string hidden_id = "##" + object.id;
                            //Checks the options list was correctly created (Exists)
                            if(!object.options.empty()){

                                //The dashboard used ImGui combo which requires a const char* array, this is built each frame currently
                                //should be a very negligiable amount of overhead
                                std::vector<const char*> items;

                                //Reserve the space required right away instead of having each push_back allocate it for a bit of optimization in the main loop
                                items.reserve(object.options.size());

                                //Converting each option to the const char* and placing in the vector
                                for (auto& option: object.options) { 
                                    items.push_back(option.c_str()); 
                                }

                                
                                if(ImGui::Combo(hidden_id.c_str(),&object.current_val_int, items.data(), (int)items.size())){

                                    // Package the ID of the widget and the selected INDEX into a JSON object
                                    nlohmann::json command_msg;
                                    command_msg["target"] = object.id;
                                    command_msg["value"]  = object.current_val_int;
    
                                    // Convert to string and send
                                    std::string payload = command_msg.dump();
                                    emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                                }
                            }
                            else {
                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No options configured for this dropdown");
                                
                                //std::cout << "In else block" << hidden_id << "\n";
                            }

                        }
                        else if (object.type == dashboardElementType::TEXTBOX){
                            
                            //To write the title of the widget (Above the widget)
                            ImGui::Text("%s", object.title.c_str());

                           
                            //Debug Message
                            //std::cout << "Check Id of textBox: " << hidden_id << "\n";
                            
                            std::string hidden_id = "##" + object.id;
                            ImGui::SetNextItemWidth(-1.0f);
                            //Returns true when user presses enter
                            if (ImGui::InputText(hidden_id.c_str(), object.text_buffer, sizeof(object.text_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                                
                                object.string_current_text = std::string(object.text_buffer);
                                
                                nlohmann::json command_msg;
                                command_msg["target"] = object.id;          
                                command_msg["value"]  = object.string_current_text; 
                                
                                std::string payload = command_msg.dump();
                                emscripten_websocket_send_utf8_text(g_WebSocket, payload.c_str());
                            }
                        }
                        else if (object.type == dashboardElementType::TEXTLABEL){
                            //Render the current text
                            ImGui::Text("%s", object.string_current_text.c_str());

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