#For keep code neat ignore
import sys
from pathlib import Path
current_dir = Path(__file__).resolve().parent
parent_dir = current_dir.parent
sys.path.append(str(parent_dir))
from generatingSignal import randomNoiseGauss
#


from random import random
import numpy as np

from bokeh.layouts import column
from bokeh.plotting import figure, curdoc

windowWidth = 3000


myFigure = figure(title="RandomNoisePlotRealTime", x_range=(0,windowWidth),y_range=(-5,5),toolbar_location=None)

scatterGraph = myFigure.scatter(x=[0],y=[0])
randomNoiseData = scatterGraph.data_source



# create a callback that adds a number in a random location
def callback():
    
    #Generate a new batch of random values
    list_newY = randomNoiseGauss(samplesPerCall=1500)
    count_new_samples = len(list_newY)
    #Need last cordinate here instead
    last_x = randomNoiseData.data['x'][-1]

    #New X axis after each update
    list_newX = np.arange(last_x + 1, last_x + count_new_samples + 1,1).tolist()
    
    #What is essentially going on here is that randomNoiseData is being treated like a FIFO buffer now
    #both the x & y will always hold 500 values before FIFO kicks in and makes way for no data  
    new_data = dict(x=list_newX,y=list_newY)
    randomNoiseData.stream(new_data,rollover=windowWidth)

    #Note, i tired to see if it would automatically move the window without me doing it dynamically
    #here i did didnt leave a range for the figure in the x-axis, it did, could be useful in ceratin cases
    #although i think defining the logic myself would always be better for debugging
    myFigure.x_range.end = randomNoiseData.data['x'][-1]
    #This could also just be an if statement when WindowWidth {randomNoiseData.data['x'][-1] - WindowWidth} 
    myFigure.x_range.start = max(0,randomNoiseData.data['x'][-1] - windowWidth)


curdoc().add_periodic_callback(callback=callback,period_milliseconds=1000)
curdoc().add_root(column(myFigure))
