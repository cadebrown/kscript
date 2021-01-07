#!/usr/bin/env ks
""" basic.ks - simple example showing usage of the Nuklear API bindings ()


SEE: https://github.com/Immediate-Mode-UI/Nuklear

@author: Cade Brown <cade@kscript.org>

"""

# Import the C bindings for Nuklear
# These are typically included in the standard library, unless explicitly disabled (or a package is missing)
import nuklear
import nx
import av

# create a context, with a given (w, h, title)
# this will be the state of the application, and will handle everything
ctx = nuklear.Context("Basic Application Window", 900, 720)


imgs = av.open("./assets/vid/rabbitman.mp4").best_video()
#imgs = av.open("../../Downloads/ex0.webp").best_video()

#imgs = mm.open("./assets/img/monarch.png")[0]
#imgs = mm.open("../../Downloads/ex0.webp")[0]


curimg = none

func nextimg() {
    ret nuklear.Image(next(imgs)) ?? none
}


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

