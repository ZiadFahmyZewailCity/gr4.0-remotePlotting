# ==========================================
# EMSCRIPTEN IMGUI DASHBOARD MAKEFILE
# ==========================================

# The Emscripten Compiler
CXX = emcc

# Output directory and the final WebAssembly HTML file
BUILD_DIR = build
TARGET = $(BUILD_DIR)/dashboard.html

# Relative path math from: dashboard/imGUI_dashboard/imGUI_Dashboard/
# ../.. lands in "dashboard/"
INCLUDES = -I../../imgui -I../../implot -I../..

# Source files
SOURCES = \
    main.cpp \
    ../../imgui/imgui.cpp \
    ../../imgui/imgui_draw.cpp \
    ../../imgui/imgui_tables.cpp \
    ../../imgui/imgui_widgets.cpp \
    ../../imgui/backends/imgui_impl_sdl2.cpp \
    ../../imgui/backends/imgui_impl_opengl3.cpp \
    ../../implot/implot.cpp \
    ../../implot/implot_items.cpp

# Emscripten Compilation Flags
EMFLAGS = -s USE_SDL=2 \
          -s USE_WEBGL2=1 \
          -lwebsocket.js \
          -s ALLOW_MEMORY_GROWTH=1 \
          # Specifically for allowing 32 bit vertex indices. Usually needed for waterfall plots
          -DImDrawIdx="unsigned int"

# ==========================================
# BUILD RULES
# ==========================================

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SOURCES)
	$(CXX) $(SOURCES) $(INCLUDES) $(EMFLAGS) -o $(TARGET)

clean:
	rm -rf $(BUILD_DIR)/*