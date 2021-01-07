/* inuklear.c - internal implementation of the Nuklear GUI library
 *
 * 
 * This includes the header, but defines 'NK_IMPLEMENTATION' which allows
 *   the library code to be emitted (only one file should have this)
 * 
 * SEE: https://immediate-mode-ui.github.io/Nuklear/doc/nuklear.html
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

/* Place code in this file */
#define NK_IMPLEMENTATION

/* xlib/OpenGL options */
#define NK_XLIB_LOAD_OPENGL_EXTENSIONS
#define NK_XLIB_GL3_IMPLEMENTATION

#define NK_GLFW_GL3_IMPLEMENTATION

#include <ks/impl.h>
#include <ks/nuklear.h>
