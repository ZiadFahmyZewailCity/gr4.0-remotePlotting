Hello, This branch contains an OOT module for GR4.0, this OOT is meant to be a proof of concept for plotting data coming from a GR flowgraph into the broswer
using bokeh 

The actual OOT module is currently simply a block called ZMQBlock.hpp which publishes its input using ZMQ to a socket 

A python script then subscribes to that soket and plots in the browswer using bokeh

To test this out

1) run mainFlowGraph in the build director of the OOT-ZMQPublisher directory 
2) while in the OOT-ZMQPublisher directory, run bokeh serve --show ContinousSinPlottingScript.py


What you should see is a broswer tab open up once you run the bokeh serve command and a sin wave be plotted 

You can also run the plotting script first to see how no data is plotted until your mainFlowGraph starts publishing data to the socket
