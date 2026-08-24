# Remote Browser Plotting/Interaction OOT module for [GNU Radio 4.0](https://github.com/gnuradio/gnuradio4)

A successor to gr-bokehgui, the out-of-tree (OOT) module developed as part of Google Summer of Code (GSoC) 2017 by Kartik Patel for GNU Radio 3.x.

This module is meant to allow for remote browser-based plotting/interaction with GNU Radio 4 flowgraphs through sinks and widgets.

Developed by Ziad Haithem Fahmi as part of GSoC 2026. 
[Development Blog](https://ziadfahmyzewailcity.github.io/blog-gr4.0-remotePlotting/)

## Requirements

- GNU Radio 4 base framework (`gnuradio4`, `gnuradio4Library`, `gnuradio4Blocks`)
- C++23-compatible compiler (e.g. `g++-14`)
- CMake ≥ 3.20
- `fmt`, `cppzmq`, `websocketpp`, `nlohmann_json`, `Threads`

## Installation Process

```bash
git clone https://github.com/ZiadFahmyZewailCity/gr4.0-remotePlotting.git
```
Then run these commands inside the OOT's repository
```bash
cmake -B build -S . -DCMAKE_CXX_COMPILER=g++-XX
cmake --build build
sudo cmake --install build
```

## Available Sinks and Widgets

### Sinks

| Sink | Description |
|---|---|
| Time Series Sink | Plots streamed samples over time |
| Frequency (FFT) Sink | Plots the frequency spectrum of a signal |
| Waterfall Sink | Plots spectrum over time as a scrolling heatmap |
| Constellation Sink | Plots I/Q samples on a constellation diagram |
| Vector Sink | Plots a fixed-length vector/array of values |

### Widgets

| Widget | Description |
|---|---|
| Button | Sends a one-shot trigger to the flowgraph on click |
| CheckBox | Sends a boolean on/off value |
| Slider | Sends a numeric value within a set range |
| Dropdown Menu | Sends a selected value from a list of options |
| Text Box | Sends a free-text string value |
| Text Label | Displays a text value from the flowgraph (read-only) |

## Link to Tutorial and Examples

TODO: Link to tutorials, example flowgraphs, and usage walkthroughs.

## Contact of GSoC contributer

Name: Ziad Haithem Fahmi
email: s-ziad.fahmy@zewailcity.edu.eg
linkedin: [Link](https://www.linkedin.com/in/ziad-fahmi-940216271/)

## License

MIT License
