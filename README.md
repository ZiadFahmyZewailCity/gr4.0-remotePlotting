Hello, This branch is purely made for familiarizing myself with how to use Bokeh and ZMQ aswell as development of OOTs for GNU Radio 4.0

There currently exists a proof of concept for plotting data coming from a GNU Radio 4.0 block to something broswer based to be accessed wirelessly

If you'd like to run this test 

Simply run 
1) "mainFlowGraph" located in OOT_TEST/Build (This generates a sin wave from signalsource blocks and publishes it to a socket using ZMQ)
2) in the terminal within folder ZMQ_realTime_SinWave run bokeh serve --show ContinousSinPlottingScript.py


What you should see is a broswer tab open up once you run the bokeh serve command and a sin wave be plotted 

You can also run the plotting script first to see how no data is plotted until your mainFlowGraph starts publishing data to the socket
