# Compiler to be used which is the emcc 
CXX = emcc

# Defining a few variables with directories
IMGUI_DIR = ../imgui
BUILD_DIR = build

# All your source files broken into a clean list
SOURCES = main.cpp \
          $(IMGUI_DIR)/imgui.cpp \
          $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# Where to look for header files
INCLUDES = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends

# Emscripten-specific web flags
LDFLAGS = -s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1

# Output file name
TARGET = $(BUILD_DIR)/index.html

# ==========================================
# Build Rules
# ==========================================

# Default rule (what runs when you type 'make')
all: $(TARGET)

# How to build the target
$(TARGET): $(SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(SOURCES) $(INCLUDES) -o $(TARGET) $(LDFLAGS)

# A handy rule to clean up generated files
clean:
	rm -f $(BUILD_DIR)/index.html $(BUILD_DIR)/index.js $(BUILD_DIR)/index.wasm $(BUILD_DIR)/index.data