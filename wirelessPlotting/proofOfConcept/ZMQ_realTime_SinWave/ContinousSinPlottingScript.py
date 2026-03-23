
import zmq
import numpy as np
from bokeh.plotting import figure, curdoc
from bokeh.layouts import column

WINDOW_WIDTH = 3200

#Creating the figure
myFigure = figure(title="Plotting Randomized sin", 
                  x_range=(0, WINDOW_WIDTH), 
                  y_range=(-15, 15), 
                  width=800, 
                  height=400,
                  toolbar_location=None)

#Plotting a line graph
linegraph = myFigure.line(x=[0], y=[0], line_width=2, color="#21a7df")
source = linegraph.data_source

#Initializing the zmp instance and socket
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://127.0.0.1:5555")
socket.setsockopt_string(zmq.SUBSCRIBE, "")

# Keep track of X across callbacks
last_x = 0

# 3. THE UPDATE LOOP
def update_plot():
    global last_x
    
    try:
        #Check socket & dont wait if there is nothing here
        raw_bytes = socket.recv(flags=zmq.NOBLOCK)
    except zmq.Again:

        return

    # Unpack the 32-bit floats
    new_y = np.frombuffer(raw_bytes, dtype=np.float32)
    num_samples = len(new_y)
    
    # Generate X coordinates
    new_x = np.arange(last_x + 1, last_x + 1 + num_samples)
    last_x = new_x[-1]
    
    # Stream the data to the plot
    new_data = dict(x=new_x.tolist(), y=new_y.tolist())
    source.stream(new_data, rollover=WINDOW_WIDTH)
    
    # Manually slide the X-axis window
    myFigure.x_range.end = last_x
    myFigure.x_range.start = max(0, last_x - WINDOW_WIDTH)


#We are checking teh socket ever 20ms and update the graph 
curdoc().add_periodic_callback(update_plot, 20)
curdoc().add_root(column(myFigure))