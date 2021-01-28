#!/usr/bin/env ks
""" nx/fft.ks - Fast Fourier Transform example


@author: Cade Brown <cade@kscript.org>
"""

import nx

# Create an array of anything
x = nx.array(range(10 ** 4))

# Get FFT(x) (across all axes)
FFT_x = nx.fft.fft(x)

print (FFT_x)

# Get IFFT(FFT(x))
IFFT_FFT_x = nx.fft.ifft(FFT_x)

# Calculate error
err = nx.sum(nx.abs(x - IFFT_FFT_x) ** 2)
print ("err:", err)
print ("digits:", nx.log(err) / nx.log(10))
