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
from bokeh.models import Button
from bokeh.palettes import RdYlBu3
from bokeh.plotting import figure, curdoc


myFigure = figure(title="RandomNoisePlotRealTime",x_range=(0, 1000), y_range=(-5, 5), toolbar_location=None)

scatterGraph = myFigure.scatter(x=[0],y=[0])

randomNoiseData = scatterGraph.data_source

# create a callback that adds a number in a random location
def callback():
    
    nextItteration = dict()

    #Generate a new batch of random values
    list_newY = randomNoiseGauss()

    #Add an equal number of x values
    number_new_samples = len(list_newY)
    #Use y or x here shouldnt matter 
    current_length = len(randomNoiseData.data['x'])
    list_newX = np.arange(current_length + 1, current_length + number_new_samples + 1,1).tolist()

    print("Original Lengths \n x:")
    print(len(randomNoiseData.data['x']))
    print("\n y:")
    print(len(randomNoiseData.data['y']))
    

    nextItteration['x'] = randomNoiseData.data['x'] + list_newX
    nextItteration['y'] = randomNoiseData.data['y'] + list_newY.tolist()
    

    randomNoiseData.data = nextItteration


#Button to generate next random number
button = Button(label="Press Me")
button.on_event('button_click', callback)

curdoc().add_root(column(button, myFigure))