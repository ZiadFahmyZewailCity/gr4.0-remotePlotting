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
// Standard C math (for things like sinf() or cosf() if you are plotting signals)
#include <math.h>

// Global variables for the window infrastructure
SDL_Window* g_Window;
SDL_GLContext g_GLContext;


void main_loop(){


    // Process input (mouse clicks, dragging)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    // This code must be run every frame to set the stage for drawing the next frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();


    //This is the actual code for the UI

    //This is the start of a window
    //Everything in the window must be put within a Begin & End
    //Interestingly this returns a boolean value, where if the window is minimized itll return a false
    //This can be used to make sure computations arent wasted on a minimized window since everything is constantly redrawn every frame
    if(ImGui::Begin("Window with some text")) {
                
        // This is basic text
        ImGui::Text("This is standard text");

        // Text but colored RGB & alpha which is opacity
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "RGB values and a value named alpha");\
        
        int number = 5;
        ImGui::Text("This function kinda works like standard printfs : %d" , number);
    }
    //Every beginning must have an end
    ImGui::End();


    if(ImGui::Begin("Window with some buttons")) {

        //Variable should be static if its to be remembered between frames
        static int clickCount = 0;

        // The button takes a label and an optional size (width, height)
        // If the button is called this stuff is executed, no callback required
        if (ImGui::Button("Click Me!", ImVec2(100, 30))) {
            clickCount++;
        }

        // Tracking button presses
        ImGui::Text("You clicked the button %d times.", clickCount);
    }
    //Every beginning must have an end
    ImGui::End();


    if(ImGui::Begin("Window with slider")){
        static float float_sliderValue = 0.5f;

        static float min_float_sliderValue = 0.0f;
        static float max_float_sliderValue = 1.0f;
        
        //This is a slides which is can be updated in real time ! perfect for the OOTs goal of allowing dynamic changes of variables
        //You pass it a reference to a variable which it updates aswell as the min & max values
        ImGui::SliderFloat("This is a slider (Float Values)", &float_sliderValue, min_float_sliderValue, max_float_sliderValue);
    
        static int int_sliderValue = 10;
        static int min_int_sliderValue = 10;
        static int max_int_sliderValue = 100;
        ImGui::SliderInt("This is a slider (Integer values)", &int_sliderValue, min_int_sliderValue, max_int_sliderValue);
    
        
    }
    //Every beginning must have an end
    ImGui::End();

    if(ImGui::Begin("Window with seperators")){
        ImGui::Button("Left Button");
        ImGui::SameLine();
        ImGui::Button("Right Button");
        ImGui::Separator();
    }
    //Every beginning must have an end
    ImGui::End();

    //The built in plotting with imGUI doesnt seem to be that good
    //Will probably have to use implot in conjuction with imGUI, imPlot seems to be an extension
    if(ImGui::Begin("Window plotting a sin wave")) {

    static float sine_data[100];
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < 100; i++) {
            sine_data[i] = sinf(i * 0.2f); 
        }
        initialized = true;
    }


    ImGui::Text("Basic Signal Output:");

    // PlotLines(Label, DataArray, NumberOfPoints, Offset, OverlayText, ScaleMin, ScaleMax, GraphSize)
    ImGui::PlotLines("##SineWave", sine_data, 100, 0, NULL, -1.0f, 1.0f, ImVec2(0, 150));

    }

    ImGui::End();

    
    // 2. Render the graphics to the browser canvas
    ImGui::Render();
    glViewport(0, 0, 1280, 720);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // The background color behind the windows
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_Window);


}

int main(){


    // Setup SDL for the browser
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    g_Window = SDL_CreateWindow("ImGui Emscripten Setup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    g_GLContext = SDL_GL_CreateContext(g_Window);
    SDL_GL_MakeCurrent(g_Window, g_GLContext);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    //This context is what essentailly tracks everything, its 
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(g_Window, g_GLContext);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // Tell Emscripten to run your main_loop function forever
    // A standard while loop would cause the browser to be unable to do anything 
    emscripten_set_main_loop(main_loop, 0, true);

    return 0;


}