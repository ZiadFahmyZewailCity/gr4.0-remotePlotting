import random
import numpy as np

class SignalGenerator:
    def __init__(self, sample_rate=32000, chunk_duration=0.05):
        self.sample_rate = sample_rate
        self.chunk_duration = chunk_duration
        self.samples_per_chunk = int(sample_rate*chunk_duration)

        #An attempt to maintain phase continuity when plotting the random signs
        self.current_phaseValue = 0

    def sinGeneration(self, amplitude=1, frequency=1000):

        #Computing the phaseVector
        timeVector = np.arange(self.samples_per_chunk)
        phaseStep = 2 * np.pi * frequency * 1/(self.sample_rate) 
        phaseVector = phaseStep * timeVector

        #We shift the phase by the previous phaseangle, this should maintain phase continutiy
        phaseVector_shifted = (phaseVector + self.current_phaseValue) % (2 * np.pi)
        sinWave = amplitude * np.sin(phaseVector_shifted)

        #If i used the last value in the phaseVector i would actually be off by one
        self.current_phaseValue = (self.current_phaseValue + (phaseStep * self.samples_per_chunk)) % (2 * np.pi)

        #Converting into seconds so it works
        seconds_timeVector = timeVector / self.sample_rate

        return seconds_timeVector, sinWave
    
    
    def dynamicSin(self, amplitude_range=[-10, 10], frequency_range=[500, 1000]):

        selected_amplitude = random.uniform(*amplitude_range)
        selected_frequency = random.uniform(*frequency_range)        

        #Computing the phaseVector
        timeVector = np.arange(self.samples_per_chunk)
        phaseStep = 2 * np.pi * selected_frequency * 1/(self.sample_rate) 
        phaseVector = phaseStep * timeVector

        #We shift the phase by the previous phaseangle, this should maintain phase continutiy
        phaseVector_shifted = (phaseVector + self.current_phaseValue) % (2 * np.pi)

        sin_wave = selected_amplitude * np.sin(phaseVector_shifted)
        
        #New final phase
        self.current_phaseValue = (self.current_phaseValue + (phaseStep * self.samples_per_chunk)) % (2 * np.pi)

        #Converting into seconds so it works
        seconds_timeVector = timeVector / self.sample_rate

        return seconds_timeVector, sin_wave

    def randomNoiseGauss(self, mean=0, standardDeviation=1):

        noise = np.random.normal(mean, standardDeviation, self.samples_per_chunk)        

        #not necessary but just if needed i return the time_vector
        seconds_time_vector = np.arange(self.samples_per_chunk) / self.sample_rate

        return seconds_time_vector, noise
