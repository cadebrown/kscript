#!/usr/bin/env ks
""" calc.ks - simple calculator using the Nuklear UI package


SEE: https://github.com/Immediate-Mode-UI/Nuklear

@author: Cade Brown <cade@kscript.org>

"""



# Nuklear GUI
import nuklear
import getarg

# create a context, with a given (w, h, title)
# this will be the state of the application, and will handle everything
ctx = nuklear.Context("Basic Application Window", 640, 480)

# calculator buttons
calc_buttons = [
    "789+",
    "456-",
    "123*",
    "C0=/"
]

# operators
ops = "+-*/"

input_str = "0"

# values
vals = (0.0, 0.0)
cur_op = ""

# perfor operation
func do_op(op, a, b) {
    if op == "" {
        # just take last value
        ret b
    } else if op == "+" {
        ret a + b
    } else if op == "-" {
        ret a - b
    } else if op == "*" {
        ret a * b
    } else if op == "/" {
        ret a / b
    } else {
        throw Error("Undefined OP!")
    }
}


# loop continuously
for frame in ctx {

    # 'begin' starts a new panel, with (x, y, w, h) size, and (optional) extra flags from the `nuklear.Window.*` enum
    # the body of the 'if' block is executed only if the window is active & visible
    if ctx.begin("Calc (4 Function)", 50, 50, 360, 320, nuklear.Window.MOVABLE | nuklear.Window.MINIMIZABLE | nuklear.Window.TITLE | nuklear.Window.NO_SCROLLBAR) {

        # create a row
        ctx.layout_row_dynamic(40, 1)

        # edit the input string
        input_str = ctx.edit_text(input_str, 64, nuklear.Edit.SIMPLE)

        # replace leading 0s
        while len(input_str) > 1 && input_str[0] == '0' && !(len(input_str) > 2 && input_str[1] == '.'), input_str = input_str[1:]

        # create a row
        ctx.layout_row_dynamic(56, 4)

        # do buttons (i, j)
        for row in calc_buttons {
            for bt in row {
                if ctx.button_label(bt) {
                    if `\d`.exact(bt) {
                        input_str = input_str + bt
                    } else if bt in ops {

                        if cur_op != "" {
                            vals[1] = do_op(cur_op, vals[1], float(input_str))
                            vals[0] = 0.0
                        } else {
                            # cycle the values
                            vals = (vals[1], float(input_str))
                        }

                        # reset the input string
                        input_str = "0"

                        cur_op = bt
                    } else if bt == "=" {
                        # cycle the values
                        vals = (vals[1], float(input_str))
                        # reset the input string
                        input_str = "0"

                        # calculate result
                        out = do_op(cur_op, vals[0], vals[1])

                        # reset cur op
                        cur_op = ""

                        # reset internal values
                        vals = (0.0, 0.0)

                        input_str = str(out)
                    } else if bt == "C" {
                        vals = (0.0, 0.0)
                        input_str = "0"
                        cur_op = ""
                    }
                }
            }
        }
    }

    # end the current panel
    ctx.end()
}

