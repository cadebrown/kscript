#!/usr/bin/env ks
""" basic.ks - simple example showing usage of the Nuklear API bindings ()


SEE: https://github.com/Immediate-Mode-UI/Nuklear

@author: Cade Brown <cade@kscript.org>

"""

# Import the C bindings for Nuklear
# These are typically included in the standard library, unless explicitly disabled (or a package is missing)
import nuklear
import nx

# create a context, with a given (w, h, title)
# this will be the state of the application, and will handle everything
ctx = nuklear.Context("Basic Application Window", 640, 480)


s = nx.rand.State()

# You can use the `for' loop, which makes it easy to continually loop every frame, until the 'x' button has been hit,
#   or there has been another action that would normally cause the application to quit
# Alternatively, you can do the more fine-graind syntax shown in the triple quote comment below:
"""
while ctx.start_frame() {
    # rendering code here
    if !ctx.end_frame(), break
}
"""
for frame in ctx {

    # 'begin' starts a new panel, with (x, y, w, h) size, and (optional) extra flags from the `nuklear.Window.*` enums
    # the body of the 'if' block is executed only if the window is active & visible
    if ctx.begin("Inner Window", 50, 50, 400, 400, nuklear.Window.TITLE | nuklear.Window.BORDER | nuklear.Window.MINIMIZABLE | nuklear.Window.MOVABLE | nuklear.Window.SCALABLE) {
    
        # create a row given (height, item_width, cols=1)
        ctx.layout_row_static(30, 80, 1)

        # on the current row, create a single button given (label)
        if ctx.button_label("My Button") {
            # if the button was pressed, the body of the if block is ran
            print ("BUTTON WAS PRESSED")
        }


        ctx.layout_row_dynamic(200, 1)
        # Output a random image
        #ctx.image(nuklear.Image(s.randf((16, 16, 3))))
    }
    ctx.end()
}
