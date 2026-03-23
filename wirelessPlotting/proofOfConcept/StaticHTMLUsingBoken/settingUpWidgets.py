#Generating the signal
import SignalGenerator
#Imports for widgets
from bokeh.models import Div, RangeSlider, Spinner

#The figure 
from bokeh.plotting import figure, output_file
#Turning file into HTML and showing it on browser
from bokeh.io import show

#Brings it all together
from bokeh.layouts import layout


#Generating my own sin wave
timeVector,sinWave = SignalGenerator.SignalGenerator.sinGeneration()

#Just naming the output file
output_file("dynamic_sin_wave.html")

#Creating a figure with a fixed range adn proportions
myFigure = figure(title="dynamic_sin", x_range=(0.0001,1), width=500, height=250)

#Adds a scater plot to figure with the time vector being the x axis and y being the amplitdue 
#of the sin
scatterSin = myFigure.scatter(x=timeVector, y=sinWave, fill_color="#21a7df")


#Web Dev stuff text
div = Div(
    text="<p>Select the circle's size using this control element:</p>",
    width=200,
    height=30,
)

#Widget of a spinner that i am using to adjust size of points on scatter plot
spinner = Spinner(
    title="Circle size",  
    low=0,  
    high=60,  
    step=5,  

    #This is the start value being set to be the same size as what the scatter plot sets it 
    #The scatter plot and spinner arent linked here
    value=scatterSin.glyph.size,  
    width=200,  
)

#Range slider i am using to vary x-axis being shown for the plot
range_slider = RangeSlider(
    title="Adjust x-axis range", 
    start=0.0001,  
    end=1,  
    step=0.0001,  
    value=(myFigure.x_range.start, myFigure.x_range.end),  
)




#This is where the actual linking, javascript is written in the background to make this happen
#Not really relevant to me it just works
spinner.js_link("value", scatterSin.glyph, "size")
range_slider.js_link("value", myFigure.x_range, "start", attr_selector=0)
range_slider.js_link("value", myFigure.x_range, "end", attr_selector=1)


#How the website should look
layout = layout([
    [div, spinner],   # Row 1: The text and the spinner sit side-by-side
    [range_slider],   # Row 2: The slider takes up the whole row below them
    [myFigure],       # Row 3: The plot sits at the very bottom
])


# Compiles the Python objects into HTML/JS, saves the file, and opens it.
show(layout)