# ==========================================
# EMSCRIPTEN IMGUI DASHBOARD MAKEFILE
# ==========================================

# The Emscripten Compiler
CXX = emcc

# Output directory and the final WebAssembly HTML file
BUILD_DIR = build
TARGET = $(BUILD_DIR)/index.html

# Include paths (Telling the compiler where to find .h and .hpp files)
# -I../imgui         : Finds "imgui.h" and "backends/..."
# -I..               : Finds "../json.hpp"
INCLUDES = -I../imgui -I..

# Source files (The actual C++ files that need to be compiled)
SOURCES = \
	main.cpp \
	../imgui/imgui.cpp \
	../imgui/imgui_draw.cpp \
	../imgui/imgui_tables.cpp \
	../imgui/imgui_widgets.cpp \
	../imgui/backends/imgui_impl_sdl2.cpp \
	../imgui/backends/imgui_impl_opengl3.cpp

# Emscripten Compilation Flags
EMFLAGS = -s USE_SDL=2 \
          -s USE_WEBGL2=1 \
          -lwebsocket.js \
          -s ALLOW_MEMORY_GROWTH=1

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