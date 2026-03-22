#For keep code neat ignore
import sys
from pathlib import Path
current_dir = Path(__file__).resolve().parent
parent_dir = current_dir.parent
sys.path.append(str(parent_dir))
from generatingSignal import sinGeneration
#

#Plotting related
from bokeh.plotting import figure, output_file
from bokeh.io import show



#Generating a sinWave to be plotted
timeVector,sinWave = sinGeneration()

output_file("sin_wave.html")


#Setting up my empty figure
figureOutline = figure(title="SinWavePlotting", x_axis_label='time (Seconds)', y_axis_label='Amplitude (Votl)')

#Adds a line plot to my figure outline
figureOutline.line(timeVector,sinWave, legend_label="DummySin",line_width=2)

show(figureOutline)

