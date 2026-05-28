# Fully AI generated just for checking outpu


import numpy as np
import matplotlib.pyplot as plt
import os

# Configuration matching your C++ flowgraph
sample_rate = 48000
input_file = "../data/signal_input.bin"
output_file = "../data/signal_output.bin"

# Quick check to make sure the files exist
if not os.path.exists(input_file) or not os.path.exists(output_file):
    print("Error: Could not find the .bin files.")
    print("Make sure you run the C++ flowgraph first, and run this script from the build/ directory.")
    exit()

# 1. Read the binary data
# We specify dtype=np.float32 because GNU Radio used <float> (32-bit)
raw_input = np.fromfile(input_file, dtype=np.float32)
processed_output = np.fromfile(output_file, dtype=np.float32)

# 2. Slice the data for visibility
# We will plot the first 0.05 seconds (2400 samples)
# A 100 Hz wave has a period of 0.01s, so this will show exactly 5 cycles.
num_samples = int(sample_rate * 0.05)

t = np.arange(num_samples) / sample_rate
input_slice = raw_input[:num_samples]
output_slice = processed_output[:num_samples]

# 3. Create the plot
plt.figure(figsize=(10, 6))

# Plot the raw input (Amplitude = 2.0)
plt.plot(t, input_slice, label='Raw Input', color='blue', linewidth=2, linestyle='--')

# Plot the processed output (Amplitude = 6.0)
plt.plot(t, output_slice, label='Processed Output (x3)', color='red', linewidth=2, alpha=0.7)

# Formatting
plt.title('GNU Radio 4.0: Input vs Output Comparison', fontsize=14)
plt.xlabel('Time (Seconds)', fontsize=12)
plt.ylabel('Amplitude', fontsize=12)
plt.grid(True, linestyle=':', alpha=0.7)
plt.legend(loc='upper right', fontsize=12)
plt.tight_layout()

# Display the plot
plt.show()