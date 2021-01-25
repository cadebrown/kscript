#!/usr/bin/env ks
""" nx/fft.ks - Fast Fourier Transform example


@author: Cade Brown <cade@kscript.org>
"""

import nx

x = nx.array(range(4))

FFTx = nx.fft.fft(x)

print (FFTx)
print (nx.la.norm(nx.fft.ifft(FFTx) - x))
#print (x - nx.fft.ifft(FFTx))

