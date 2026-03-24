##THIS FILE WAS MADE BY AI JUST FOR RAPID TESTING

import numpy as np
import matplotlib.pyplot as plt

# 1. Read the raw binary file
# np.complex64 tells NumPy to read 8 bytes at a time (4 for real, 4 for imag)
data = np.fromfile('build/output.iq', dtype=np.complex64)

print(f"Successfully read {len(data)} complex samples.")

# 2. Plot the first 100 samples to see the waveform clearly
# (At 32 ksps, a 1 kHz wave takes 32 samples per full cycle, so 100 samples shows ~3 cycles)
plt.figure(figsize=(10, 4))
plt.plot(np.real(data[:100]), label='In-Phase (Real - I)', marker='.')
plt.plot(np.imag(data[:100]), label='Quadrature (Imaginary - Q)', marker='.')

plt.title('1 kHz Complex Sine Wave (32 ksps)')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.legend(loc='upper right')
plt.grid(True)
plt.tight_layout()

plt.show()