#!/usr/bin/env ks
""" showfft.ks - simple example showing FFT of an image


SEE: https://github.com/Immediate-Mode-UI/Nuklear

@author: Cade Brown <cade@kscript.org>

"""

# Import the C bindings for Nuklear
# These are typically included in the standard library, unless explicitly disabled (or a package is missing)
import nuklear
import nx
import av
import m

# create a context, with a given (w, h, title)
# this will be the state of the application, and will handle everything
ctx = nuklear.Context("Basic Application Window", 900, 720)

# Process an image
func proc(x) {
    x = x[..., :3]

    # Generate convolution kernel
    K = 53
    sigma = K * .00001

    k = nx.zeros(x.shape)

    #off = (K - 1) / 2
    #for i in range(K) {
    #    for j in range(K) {
    #        v = m.exp(-((i - off) ** 2 + (j - off) ** 2) / (2 * sigma ** 2)) / m.sqrt(2 * m.pi * sigma ** 2)
    #        k[(i + 8) % K, j, 0] = v
    #        k[i, j] = v
    #        k[i, (j + 50) % K, 2] = v
    #    }
    #}
    # Normalize per color
    
    k[0, 0, 0] = 1
    k[50, 50, 1] = 1
    k[25, 80, 2] = 1
    #k /= nx.sum(k) / 3

    # Now, apply the kernel
    Fx = nx.fft.fft(x, (0, 1))
    Fk = nx.fft.fft(k, (0, 1))

    # Now, compute the FFT of the result
    Fxk = Fx * Fk

    # Inverse that FFT, and re-scale
    xk = nx.fft.ifft(Fxk, (0, 1))

    ret xk
}


func nextimg() {
    ret nuklear.Image(proc(next(imgs))) ?? none
}


imgs = av.open("./assets/video/rabbitman.mp4").best_video()

curimg = none

for frame in ctx {

    # 'begin' starts a new panel, with (x, y, w, h) size, and (optional) extra flags from the `nuklear.Window.*` enums
    # the body of the 'if' block is executed only if the window is active & visible
    if ctx.begin("Inner Window", 50, 50, 600, 600, nuklear.Window.TITLE | nuklear.Window.BORDER | nuklear.Window.MINIMIZABLE | nuklear.Window.MOVABLE | nuklear.Window.SCALABLE) {
    
        # create a row given (height, item_width, cols=1)
        ctx.layout_row_static(30, 80, 1)

        # on the current row, create a single button given (label)
        if ctx.button_label("My Button") {
            # if the button was pressed, the body of the if block is ran
            print ("BUTTON WAS PRESSED")
        }


        ctx.layout_row_dynamic(360, 1)
        # Output a random image
        curimg = nextimg() || curimg
        ctx.image(curimg)
    }
    ctx.end()
}

