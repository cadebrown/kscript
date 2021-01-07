/* ks/nuklear.h - header for the 'nuklear' (nuklear GUI) module
 * 
 * The porefix is 'ksnk' instead of something longer, just to save space in the C API
 * 
 * 
 * SEE: https://github.com/vurtun/nuklear/issues/226
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNK_H__
#define KSNK_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/* Numerics library */
#include <ks/nx.h>


/* Configuration */


#if defined(KS_HAVE_x) && defined(KS_HAVE_opengl)
#define KSNK_DO 1

/* Declare the type of nuklear bindings we are using */
#define KSNK_XLIB_GL3


/* Nuklear options */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT


/* Xlib */
#include <X11/Xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

/* gl3w */
#ifndef NK_IMPLEMENTATION
#include <ks/igl3w_gl3.h>
#endif

/* OpenGL */
#include <GL/glx.h>

/* Nuklear */
#include <ks/inuklear.h>
#include <ks/inuklear_xlib_gl3.h>


#elif defined(HS_KAVE_glfw)
#define KSNK_DO 1


/* Declare the type of nuklear bindings we are using */
#define KSNK_GLFW_GL3

/* Nuklear options */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT

/* OpenGL */
#include <ks/igl3w_gl3.h>

/* GLFW */
#include <GLFW/glfw3.h>

/* Nuklear */
#include <ks/inuklear.h>
#include <ks/inuklear_glfw_gl3.h>

#endif



/* Types */


/* nuklear.Context - Rendering context, which is an application window
 *
 * 
 */
typedef struct ksnk_Context_s {
    KSO_BASE

    /* Number of frames so far */
    int n_frames;


    /* Background color */
    float col_bk[3];

#ifdef KSNK_DO
    /* Nuklear context */
    struct nk_context* ctx;

    /* Fonts used by the context */
    struct nk_font_atlas* atlas;
#endif

#if defined(KSNK_XLIB_GL3)

    XVisualInfo* vis;

    Colormap cmap;

    /* Xlib specific */
    struct {

        /* OpenGL context */
        GLXContext glx_ctx;

        /* OpenGL framebuffer config */
        GLXFBConfig fbc;

        Display* display;
        Window window;

        XVisualInfo *vis;
        Colormap cmap;
        XSetWindowAttributes swa;
        XWindowAttributes attr;
        Atom wm_delete_window;

    } x;

#elif defined(KSNK_GLFW_GL3)

    /* GLFW Window */
    GLFWwindow* window;

#endif

}* ksnk_Context;


/* nuklear.Image - displayable image handle
 *
 */
typedef struct ksnk_Image_s {
    KSO_BASE
#ifdef KSNK_DO

    /* Nuklear object */
    struct nk_image val;

#endif

    /* OpenGL texture to free (or negative to not free) */
    int gltex;

}* ksnk_Image;



/* Functions */


/* Create an image handle from an 'nxar_t'
 */
KS_API ksnk_Image ksnk_Image_new(ks_type tp, nxar_t img);



/* Types */
KS_API extern ks_type
    ksnkt_Context,
    ksnkt_Image
;

/* Enums */
KS_API extern ks_type 
    ksnke_Heading, 
    ksnke_Symbol, 
    ksnke_Window,
    ksnke_Key,
    ksnke_Button, 
    ksnke_Edit
;

#endif /* KSNK_H__ */
