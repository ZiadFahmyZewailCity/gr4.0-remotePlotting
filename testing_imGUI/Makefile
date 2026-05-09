# 1. Compiler setting
CXX = emcc

# 2. All your source files broken into a clean list
SOURCES = main.cpp \
          imgui/imgui.cpp \
          imgui/imgui_draw.cpp \
          imgui/imgui_widgets.cpp \
          imgui/imgui_tables.cpp \
          imgui/backends/imgui_impl_sdl2.cpp \
          imgui/backends/imgui_impl_opengl3.cpp

# 3. Where to look for header files
INCLUDES = -Iimgui -Iimgui/backends

# 4. Emscripten-specific web flags
LDFLAGS = -s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1

# 5. Output file name
TARGET = index.html

# ==========================================
# Build Rules
# ==========================================

# Default rule (what runs when you type 'make')
all: $(TARGET)

# How to build the target
$(TARGET): $(SOURCES)
	$(CXX) $(SOURCES) $(INCLUDES) -o $(TARGET) $(LDFLAGS)

# A handy rule to clean up generated files
clean:
	rm -f index.html index.js index.wasm