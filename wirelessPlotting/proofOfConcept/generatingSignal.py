


import numpy as np

#Generating a sin wave
def sinGeneration(amplitude = 1, duration = 0.002, frequency = 1000, sample_rate = 32000):
    timeVector = np.arange(0,duration,1/sample_rate)
    sinWave = amplitude * np.sin(2 * np.pi * frequency * timeVector)
    return timeVector,sinWave

#Generating noise
#Returns a numpy array of random values
def randomNoiseGauss(mean = 0, standardDeviation = 1,samplesPerCall = 100):
    return np.random.normal(mean,standardDeviation,samplesPerCall)
