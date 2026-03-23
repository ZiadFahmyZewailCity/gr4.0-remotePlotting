#For keep code neat ignore
import sys
from pathlib import Path
current_dir = Path(__file__).resolve().parent
parent_dir = current_dir.parent
sys.path.append(str(parent_dir))
from SignalGenerator import SignalGenerator

import zmq
import time
import numpy as np


#Creates a socket
context = zmq.Context()
#Intialize a socket for publishing
socket = context.socket(zmq.PUB)
#Assign a specific tcp socket for our context
socket.bind("tcp://127.0.0.1:5555")

# Time for intialization, recommened,  initialize before blasting data
time.sleep(1)

sig_gen = SignalGenerator(sample_rate=32000,chunk_duration=1)


try:
    while True:
        timerVector,sinValues = sig_gen.dynamicSin()

        raw_data = sinValues.astype(np.float32).tobytes()
        socket.send(raw_data)
        time.sleep(sig_gen.chunk_duration)

except KeyboardInterrupt:
    print("\ninterruptRecvied")
finally:
    socket.close()
    context.term()

