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


imgsrc = av.imread("../../Downloads/download.png")
#imgsrc = av.imread("./assets/image/monarch.png")
imgsrc = imgsrc[..., :3]
imgsrc = nx.fft.fft(imgsrc, (0, 1))


kern = nx.zeros((imgsrc.shape[0], imgsrc.shape[1], 3))

K = 23
sigma = 5

for i in range(K) {
    for j in range(K) {
        v = m.exp(-((i - K // 2) ** 2 + (j - K // 2) ** 2) / (2 * sigma ** 2)) / m.sqrt(2 * m.pi * sigma ** 2)
        kern[(i + 8) % K, j, 0] = v
        kern[i, j, 1] = v
        kern[i, (j + 20) % K, 2] = v
    }
}
kern /= nx.sum(kern)
#kern /= kern[K//2, K//2]


Fk = nx.fft.fft(kern, (0, 1))

imgsrc = imgsrc * Fk
#imgsrc = imgsrc * Fk

imgsrc = nx.fft.ifft(imgsrc, (0, 1)) 

#imgsrc = nx.fft.ifft(imgsrc, (0, 1))
#imgsrc = nx.abs(nx.fft.fft(imgsrc, (0, 1))) / (imgsrc.shape[0] + imgsrc.shape[1])
#imgsrc = nx.log(1 + imgsrc) * 30
#imgsrc = nx.fft.ifft(imgsrc, (0, 1))

#imgsrc = nx.log(nx.abs(nx.fft.fft(imgsrc, (0, 1)))) / 50
img = nuklear.Image(imgsrc)

#imgs = av.open("./assets/vid/rabbitman.mp4").best_video()
#imgs = av.open("../../Downloads/ex0.webp").best_video()

#imgs = mm.open("./assets/img/monarch.png")[0]
#imgs = mm.open("../../Downloads/ex0.webp")[0]


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
        #curimg = nextimg() || curimg
        ctx.image(img)
    }
    ctx.end()
}

