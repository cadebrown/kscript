/* main.c - source code for the built-in 'nuklear' module
 *
 * 
 * SEE: https://immediate-mode-ui.github.io/Nuklear/doc/nuklear.html
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/mm.h>
#include <ks/cext.h>

#define M_NAME "nuklear"

#define TC_NAME M_NAME ".Context"
#define TT_NAME M_NAME ".Image"


/* Detect if we have the correct packages */
#if defined(KS_HAVE_x) && defined(KS_HAVE_opengl)
  #define BUILD_CNK
  #define CNK_USE_XLIB
#endif

//#define CNK_USE_GLFW


#ifdef BUILD_CNK
/* Actually build this module */



#if defined(CNK_USE_XLIB)

/* Implement via Xlib, OpenGL3 */
#define NK_XLIB_GL3_IMPLEMENTATION
#define NK_XLIB_LOAD_OPENGL_EXTENSIONS

/* Nuklear options */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT

/* Place code in this file */
#define NK_IMPLEMENTATION

/* Nuklear headers */
#include "./ext/nuklear.h"
#include "./ext/nuklear_xlib_gl3.h"


#elif defined(CNK_USE_GLFW)


/* Implement via GLFW, OpenGL3 */
#define NK_GLFW_GL3_IMPLEMENTATION

/* Nuklear options */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT


/* Place code in this file */
#define NK_IMPLEMENTATION

/* Include OpenGL/GLFW */
#include "./ext/gl3w_gl3.h"
#include <GLFW/glfw3.h>

/* Nuklear headers */
#include "./ext/nuklear.h"
#include "./ext/nuklear_glfw_gl3.h"

#endif



/* Definitions */

struct ks_type_s tp_ctx;
ks_type kscnkt_ctx = &tp_ctx;

struct ks_type_s tp_image;
ks_type kscnkt_image = &tp_image;

/* Enumerations */
ks_type kscnke_heading = NULL, kscnke_symbol = NULL, kscnke_window = NULL, kscnke_key = NULL, kscnke_button = NULL, kscnke_edit = NULL;


/* Types */

/* nuklear.Context - wraps the Nuklear's Context, as
 *                     well as a window
 *
 */
typedef struct cnk_ctx_s {
    KSO_BASE

    /* Number of frames so far */
    int n_frames;

    /* Nuklear context */
    struct nk_context* ctx;

    /* Fonts used by the context */
    struct nk_font_atlas* atlas;

    /* Background color */
    float col_bk[3];


#if defined(CNK_USE_XLIB)

    /* X-11 specific */
    struct {

        /* OpenGL context */
        GLXContext glCTX;


        Display* display;

        Window window;

        XVisualInfo *vis;
        Colormap cmap;
        XSetWindowAttributes swa;
        XWindowAttributes attr;
        GLXFBConfig fbc;
        Atom wm_delete_window;
        int width, height;

    } x;

#elif defined(CNK_USE_GLFW)

    /* GLFW Window */
    GLFWwindow* window;

#endif

}* cnk_ctx;


/* nuklear.Texture - represents an image texture which can be displayed
 *
 */
typedef struct cnk_image_s {
    KSO_BASE

    /* Value of the texture */
    struct nk_image val;


    /* OpenGL texture to free (or negative to not free) */
    int gltex;

}* cnk_image;


/* Internals */

/* Versions of OpenGL to try to intiialize with */
static int I_glvers[][2] = {
    {3, 3},
    {-1, -1},
};

/* Major and minor versions used */
static int glMajor = -1, glMinor = -1;

/* Maximum vertex buffer size */
static int cnk_max_verbuf = 512 * 1024;

/* Maximum element buffer size */
static int cnk_max_elebuf = 128 * 1024;



#if defined(CNK_USE_XLIB)

/* Xlib call back function for errors */
static int _X_errorcb(Display* display, XErrorEvent* ev) {
    char tmp[256];
    XGetErrorText(display, ev->error_code, tmp, sizeof(tmp) - 1);
    ks_warn("ks", "[X11]: %s\n", tmp);
    return 0;
}

/* Return whether or not an extension string has a given extension */
static bool _X_hasext(const char *string, const char *ext) {
    const char *start, *where, *term;
    where = strchr(ext, ' ');
    if (where || *ext == '\0')
        return false;

    for (start = string;;) {
        where = strstr((const char*)start, ext);
        if (!where) break;
        term = where + strlen(ext);
        if (where == start || *(where - 1) == ' ') {
            if (*term == ' ' || *term == '\0')
                return true;
        }
        start = term;
    }

    return false;
}

#elif defined(CNK_USE_GLFW)

/* Error callback for GLFW */
static void _GLFW_errorcb(int er, const char* d) {
    ks_warn("ks", "GLFW: %s [code: %i]", d, er);
}

/* Key input callback for GLFW */
static void _GLFW_keycb(GLFWwindow *win, unsigned int codepoint) {
    cNk_Context ctx = (cNk_Context)glfwGetWindowUserPointer(win);
    nk_input_unicode(ctx->ctx, codepoint);
}

#endif


/* C-API */


#define GL_CHK() do { \
    int err = glGetError(); \
    if (err) { \
        ks_warn("ks", "OpenGL error: %i", err); \
    } \
} while (0)


/* Create a new texture from 'img' source */
cnk_image kscnk_image_new(ks_type tp, nxar_t img) {
    int w, h, d;

    /* RGBA data */
    nxc_float* data = NULL;

    if (img.rank == 2) {
        /* Black and white */
        h = img.dims[0];
        w = img.dims[1];
        d = 1;
        data = ks_malloc(sizeof(*data) * w * h * 4);
        if (!nx_cast(
            (nxar_t) {
                data,
                nxd_float,
                3,
                (ks_size_t[]){ h, w, 3 },
                (ks_ssize_t[]){ 4 * w * sizeof(*data), 4 * sizeof(*data), 1 * sizeof(*data) },
            },
            (nxar_t) {
                img.data,
                img.dtype,
                3,
                (ks_size_t[]) { img.dims[0], img.dims[1], 1 },
                (ks_ssize_t[]) { img.strides[0], img.strides[1], 0 },
            }
        )) {
            ks_free(data);
            return NULL;
        }

        /* Set alpha channel */
        if (!nx_cast(
            (nxar_t) {
                &data[3],
                nxd_float,
                3,
                (ks_size_t[]){ h, w, 1 },
                (ks_ssize_t[]){ 4 * w * sizeof(*data), 4 * sizeof(*data), 1 * sizeof(*data) },
            },
            (nxar_t) {
                (nxc_float[]){ 1.0 },
                nxd_float,
                0,
                NULL,
                NULL
            }
        )) {
            ks_free(data);
            return NULL;
        }



    } else if (img.rank == 3) {
        /* Color */
        h = img.dims[0];
        w = img.dims[1];
        d = img.dims[2];

        if (d == 1) {
            /* Grayscale */
        } else if (d == 3) {
            /* RGB */
        } else if (d == 4) {
            /* RGBA */
        } else {
            KS_THROW(kst_Error, "Images with a channels dimension must have 1, 3, or 4 channels (image had %i)", d);
            return NULL;
        }

        data = ks_malloc(sizeof(*data) * w * h * 4);
        if (!nx_cast(
            (nxar_t) {
                data,
                nxd_float,
                3,
                (ks_size_t[]){ h, w, 4 },
                (ks_ssize_t[]){ 4 * w * sizeof(*data), 4 * sizeof(*data), 1 * sizeof(*data) },
            },
            img
        )) {
            ks_free(data);
            return NULL;
        }

    } else {
        KS_THROW(kst_Error, "Images must be created from rank-2 or rank-3 arrays (not rank-%i)", img.rank);
        return NULL;
    }


    assert(data != NULL);

    cnk_image self = KSO_NEW(cnk_image, tp);

    /* Generate OpenGL texture */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    /* Set reasonable default texture parameters */

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    /*
    GLfloat largest_supported_anisotropy; 
    glGetFloatv(GL_MAX_IMAGE_MAX_ANISOTROPY, &largest_supported_anisotropy); 
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, largest_supported_anisotropy);
    */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data);
    glGenerateMipmap(GL_TEXTURE_2D); 


    ks_free(data);
    /* Construct a wrapper for the Nuklear image */
    self->val = nk_image_id((int)tex);

    return self;
}


/* Type Functions */


static KS_TFUNC(TT, free) {
    cnk_image self;
    KS_ARGS("self:*", &self, kscnkt_image);

    if (self->gltex >= 0) {
        GLuint tex = self->gltex;
        glDeleteTextures(1, &tex);
    }

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TT, new) {
    ks_type tp;
    kso img;
    KS_ARGS("tp:* img", &tp, kst_type, &img);

    nxar_t ar;
    kso rr;
    if (!nxar_get(img, NULL, &ar, &rr)) {
        return NULL;
    }

    cnk_image self = kscnk_image_new(tp, ar);
    KS_NDECREF(rr);

    return (kso)self;
}

static KS_TFUNC(C, free) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    /* Clean up Nuklear objects */
    nk_free(self->ctx);
    nk_font_atlas_cleanup(self->atlas);

#if defined(CNK_USE_XLIB)
    XFree(self->x.vis);

#elif defined(CNK_USE_GLFW)

    /* Destroy the GLFW window */
    glfwDestroyWindow(self->window);

#endif


    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(C, new) {
    ks_type tp;
    ks_str name;
    ks_cint w, h;
    KS_ARGS("tp:* name:* w:cint h:cint", &tp, kst_type, &name, kst_str, &w, &h);

    cnk_ctx self = KSO_NEW(cnk_ctx, tp);
    self->n_frames = 0;

    self->col_bk[0] = self->col_bk[1] = self->col_bk[2] = 0.1f;

#if defined(CNK_USE_XLIB)

    if (!(self->x.display = XOpenDisplay(NULL))) {
        KS_THROW(kst_Error, "Failed to open Xlib display");
        return NULL;
    }

    int glx_major, glx_minor;
    if (!glXQueryVersion(self->x.display, &glx_major, &glx_minor)) {
        KS_THROW(kst_Error, "Failed to query OpenGL version");
        return NULL;
    }

    ks_debug("ks", "[X11]: Queried OpenGL version %i.%i\n", glx_major, glx_minor);

    /* find and pick matching framebuffer visual */
    int fb_count;
    static GLint attr[] = {
        GLX_X_RENDERABLE,   True,
        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,    GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_ALPHA_SIZE,     8,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   8,
        GLX_DOUBLEBUFFER,   True,
        None
    };

    GLXFBConfig *fbc;
    fbc = glXChooseFBConfig(self->x.display, DefaultScreen(self->x.display), attr, &fb_count);
    if (!fbc) {
        KS_THROW(kst_Error, "Failed to retrieve framebuffer config from Xlib");
        return NULL;
    }

    // pick the configuration with the most samples/pixel
    int i;
    int fb_best = -1, best_num_samples = -1;
    for (i = 0; i < fb_count; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(self->x.display, fbc[i]);
        if (vi) {
            int sample_buffer, samples;
            glXGetFBConfigAttrib(self->x.display, fbc[i], GLX_SAMPLE_BUFFERS, &sample_buffer);
            glXGetFBConfigAttrib(self->x.display, fbc[i], GLX_SAMPLES, &samples);
            if ((fb_best < 0) || (sample_buffer && samples > best_num_samples))
                fb_best = i, best_num_samples = samples;
        }
        XFree(vi);
    }
    self->x.fbc = fbc[fb_best];
    XFree(fbc);
    self->x.vis = glXGetVisualFromFBConfig(self->x.display, self->x.fbc);

    /* Create a colormap for the window */
    self->x.cmap = XCreateColormap(self->x.display, RootWindow(self->x.display, self->x.vis->screen), self->x.vis->visual, AllocNone);
    self->x.swa.colormap =  self->x.cmap;
    self->x.swa.background_pixmap = None;
    self->x.swa.border_pixel = 0;
    self->x.swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
        PointerMotionMask| StructureNotifyMask;
    
    // create window
    self->x.window = XCreateWindow(self->x.display, RootWindow(self->x.display, self->x.vis->screen), 0, 0,
        w, h, 0, self->x.vis->depth, InputOutput,
        self->x.vis->visual, CWBorderPixel|CWColormap|CWEventMask, &self->x.swa
    );

    if (!self->x.window) {
        KS_THROW(kst_Error, "Failed to create window in Xlib");
        return NULL;
    }
    
    XFree(self->x.vis);

    /* Set data about window */
    XStoreName(self->x.display, self->x.window, name->data);
    XMapWindow(self->x.display, self->x.window);
    self->x.wm_delete_window = XInternAtom(self->x.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(self->x.display, self->x.window, &self->x.wm_delete_window, 1);

    /* OpenGL context configuration */


    // error handler
    int (*old_handler)(Display*, XErrorEvent*) = XSetErrorHandler(_X_errorcb);

    // Create context FP
    typedef GLXContext (*glxCreateContext)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    // load extensions for OpenGL
    const char *extensions_str = glXQueryExtensionsString(self->x.display, DefaultScreen(self->x.display));

    // Attempt to get the context
    glxCreateContext create_context = (glxCreateContext)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

    // whether or not we had an error
    bool hadErr = false;

    if (!_X_hasext(extensions_str, "GLX_ARB_create_context") || !create_context) {
        ks_info("ks", "[X11]: glxCreateContextAttribARB() was not found, so using old style GLX context");
        self->x.glCTX = glXCreateNewContext(self->x.display, self->x.fbc, GLX_RGBA_TYPE, 0, True);
    } else {
        GLint attr[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            None
        };
        self->x.glCTX = create_context(self->x.display, self->x.fbc, 0, True, attr);
        XSync(self->x.display, False);
        if (hadErr || !self->x.glCTX) {
            /* Could not create GL 3.0 context. Fallback to old 2.x context.
                * If a version below 3.0 is requested, implementations will
                * return the newest context version compatible with OpenGL
                * version less than version 3.0.*/
            attr[1] = 1; attr[3] = 0;
            hadErr = false;
            ks_info("ks", "[X11]: Failed to create OpenGL 3.0 context, using old style GLX context");
            self->x.glCTX = create_context(self->x.display, self->x.fbc, 0, True, attr);
        }
    }


    XSync(self->x.display, False);
    XSetErrorHandler(old_handler);
    if (hadErr || !self->x.glCTX) {
        KS_THROW(kst_Error, "Failed to create an OpenGL context in Xlib");
        return NULL;
    }

    /* Make it the current context */
    glXMakeCurrent(self->x.display, self->x.window, self->x.glCTX);

    /* Now, initialize nuklear */
    self->ctx = nk_x11_init(self->x.display, self->x.window);

    /* load font assets */
    nk_x11_font_stash_begin(&self->atlas);

    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/

    nk_x11_font_stash_end();


#elif defined(CNK_USE_GLFW)

    // construct a new GLFW window with the given parameters
    self->window = glfwCreateWindow(width, height, title->chr, NULL, NULL);

    glfwMakeContextCurrent(self->window);
    glfwSetCharCallback(self->window, _GLFW_keycb);

    // set user data to the context object
    glfwSetWindowUserPointer(self->window, self);

    // create a context from the GLFW window
    self->ctx = nk_glfw3_init(self->window, NK_GLFW3_INSTALL_CALLBACKS);

    /* load font assets */
    nk_glfw3_font_stash_begin(&self->atlas);

    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/

    nk_glfw3_font_stash_end();

#endif


    /* load cursor assets */

    // nk_style_load_all_cursors(ctx, atlas->cursors);

    return (kso)self;
}



static KS_TFUNC(C, getattr) {
    cnk_ctx self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kscnkt_ctx, &attr, kst_str);

    if (ks_str_eq_c(attr, "size", 4)) {
        /* Return (w, h) tuple of the window size */
        int ww, wh;

#if defined(CNK_USE_XLIB)
        XGetWindowAttributes(self->x.display, self->x.window, &self->x.attr);
        ww = self->x.attr.width;
        wh = self->x.attr.height;
#elif defined(CNK_USE_GLFW)
        glfwGetWindowSize(self->window, &ww, &wh);
#endif

        return (kso)ks_tuple_newn(2, (kso[]){
            (kso)ks_int_new(ww),
            (kso)ks_int_new(wh)
        });
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


static KS_TFUNC(C, setattr) {
    cnk_ctx self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, kscnkt_ctx, &attr, kst_str, &val);

    if (ks_str_eq_c(attr, "size", 4)) {
        ks_tuple wh = ks_tuple_newi(val);
        if (!wh) return NULL;

        if (wh->len != 2) {
            KS_THROW(kst_SizeError, "Expected iterable of length for 'size', but got one of length %u", wh->len);
            KS_DECREF(wh);
            return NULL;
        }

        ks_cint w, h;
        if (!kso_get_ci(wh->elems[0], &w) || !kso_get_ci(wh->elems[1], &h)) {
            KS_DECREF(wh);
            return NULL;
        }

        KS_DECREF(wh);

        /* Set the size */
#if defined(CNK_USE_XLIB)
        XResizeWindow(self->x.display, self->x.window, w, h);
#elif defined(CNK_USE_GLFW)
        glfwSetWindowSize(self->window, w, h);
#endif

        return KSO_NONE;
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


/** Nuklear API **/

/*** Iteration/Drawing ***/



/*** Drawing ***/

static KS_TFUNC(C, start_frame) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    bool rst = true;

#if defined(CNK_USE_XLIB)

    // TODO: seperate events maybe?
    XEvent evt;
    nk_input_begin(self->ctx);
    while (XPending(self->x.display)) {
        XNextEvent(self->x.display, &evt);
        if (evt.type == ClientMessage) rst = false;
        if (XFilterEvent(&evt, self->x.window)) continue;
        nk_x11_handle_event(&evt);
    }

    nk_input_end(self->ctx);

#elif defined(CNK_USE_GLFW)

    glfwPollEvents();
    nk_glfw3_new_frame();
    if (glfwWindowShouldClose(self->window)) rst = false;

#endif

    return KSO_BOOL(rst);
}

static KS_TFUNC(C, end_frame) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    bool rst = true;

    /* Query the size */
    int dw, dh;
#if defined(CNK_USE_XLIB)
    XGetWindowAttributes(self->x.display, self->x.window, &self->x.attr);
    dw = self->x.attr.width;
    dh = self->x.attr.height;
#elif defined(CNK_USE_GLFW)
    glfwGetWindowSize(self->window, &dw, &dh);
#endif


    /* Shared OpenGL code */

    /* Render the whole window */
    glViewport(0, 0, dw, dh);

    /* Set background color */
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(self->col_bk[0], self->col_bk[1], self->col_bk[2], self->col_bk[3]);


#if defined(CNK_USE_XLIB)

    nk_x11_render(NK_ANTI_ALIASING_ON, cnk_max_verbuf, cnk_max_elebuf);
    glXSwapBuffers(self->x.display, self->x.window);

#elif defined(CNK_USE_GLFW)
    // use the Nuklear GLFW3 render helper (with our configuration of max memory)
    nk_glfw3_render(NK_ANTI_ALIASING_ON, _NK_max_verbuf, _NK_max_elebuf);

    // swap buffers
    glfwSwapBuffers(self->window);

    if (glfwWindowShouldClose(self->window)) rst = false;
#endif

    /* Return whether it should keep going */
    return KSO_BOOL(rst);
}


static KS_TFUNC(C, next) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    bool t;
    kso r;

    if (self->n_frames > 0) {
        r = C_end_frame_(1, (kso[]){ (kso)self });
        if (!r) return NULL;
        
        if (!kso_truthy(r, &t)) {
            KS_DECREF(r);
            return NULL;
        }

        KS_DECREF(r);

        /* Quit now */
        if (!t) {
            KS_OUTOFITER();
            return NULL;
        }
    }

    r = C_start_frame_(1, (kso[]){ (kso)self });
    if (!r) return NULL;

    if (!kso_truthy(r, &t)) {
        KS_DECREF(r);
        return NULL;
    }

    KS_DECREF(r);

    /* Quit now */
    if (!t) {
        KS_OUTOFITER();
        return NULL;
    }

    return (kso)ks_int_new(self->n_frames++);
}


/*** Input ***/


/*** State ***/

static KS_TFUNC(C, isfocused) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    int res = nk_window_has_focus(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, ishovered) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    int res = nk_window_is_hovered(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, iscollapsed) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    int res = nk_window_is_collapsed(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, isclosed) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    int res = nk_window_is_closed(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, ishidden) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    int res = nk_window_is_hidden(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, isactive) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    int res = nk_window_is_active(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, isanyactive) {
    cnk_ctx self;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx);

    int res = nk_item_is_any_active(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, set_bounds) {
    cnk_ctx self;
    ks_str name;
    ks_cfloat x, y, w, h;
    KS_ARGS("self:* name:* x:cfloat y:cfloat w:cfloat h:cfloat", &self, kscnkt_ctx, &name, kst_str, &x, &y, &w, &h);

    nk_window_set_bounds(self->ctx, name->data, nk_rect(x, y, w, h));

    return KSO_NONE;
}

static KS_TFUNC(C, set_pos) {
    cnk_ctx self;
    ks_str name;
    ks_cfloat x, y;
    KS_ARGS("self:* name:* x:cfloat y:cfloat", &self, kscnkt_ctx, &name, kst_str, &x, &y);

    nk_window_set_position(self->ctx, name->data, nk_vec2(x, y));

    return KSO_NONE;
}

static KS_TFUNC(C, set_size) {
    cnk_ctx self;
    ks_str name;
    ks_cfloat w, h;
    KS_ARGS("self:* name:* w:cfloat h:cfloat", &self, kscnkt_ctx, &name, kst_str, &w, &h);

    nk_window_set_size(self->ctx, name->data, nk_vec2(w, h));

    return KSO_NONE;
}

static KS_TFUNC(C, set_scroll) {
    cnk_ctx self;
    ks_str name;
    ks_uint x, y;
    KS_ARGS("self:* name:* x:cint y:cint", &self, kscnkt_ctx, &name, kst_str, &x, &y);

    nk_window_set_scroll(self->ctx, x, y);

    return KSO_NONE;
}

static KS_TFUNC(C, focus) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_set_focus(self->ctx, name->data);

    return KSO_NONE;
}

static KS_TFUNC(C, close) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_close(self->ctx, name->data);

    return KSO_NONE;
}

static KS_TFUNC(C, minimize) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_collapse(self->ctx, name->data, NK_MINIMIZED);

    return KSO_NONE;
}

static KS_TFUNC(C, maximize) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_collapse(self->ctx, name->data, NK_MAXIMIZED);

    return KSO_NONE;
}

static KS_TFUNC(C, show) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_show(self->ctx, name->data, NK_SHOWN);

    return KSO_NONE;
}

static KS_TFUNC(C, hide) {
    cnk_ctx self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kscnkt_ctx, &name, kst_str);

    nk_window_show(self->ctx, name->data, NK_HIDDEN);

    return KSO_NONE;
}


/*** Layout ***/

static KS_TFUNC(C, begin) {
    cnk_ctx self;
    ks_str title;
    ks_cfloat x, y, w, h;
    ks_cint flags;
    KS_ARGS("self:* title:* x:cfloat y:cfloat w:cfloat h:cfloat ?flags:cint", &self, kscnkt_ctx, &title, kst_str, &x, &y, &w, &h, &flags);

    int res = nk_begin(self->ctx, title->data, nk_rect(x, y, w, h), flags);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, end) {
    cnk_ctx self;
    KS_ARGS("self:*", &self, kscnkt_ctx);

    nk_end(self->ctx);
   
    return KSO_NONE;
}

static KS_TFUNC(C, layout_row_static) {
    cnk_ctx self;
    ks_cfloat height;
    ks_cint width, cols;
    KS_ARGS("self:* height:cfloat width:cint ?cols:cint", &self, kscnkt_ctx, &height, &width, &cols);

    nk_layout_row_static(self->ctx, height, width, cols);

    return KSO_NONE;
}

static KS_TFUNC(C, layout_row_dynamic) {
    cnk_ctx self;
    ks_cfloat height;
    ks_cint cols;
    KS_ARGS("self:* height:cfloat ?cols:cint", &self, kscnkt_ctx, &height, &cols);

    nk_layout_row_static(self->ctx, height, height, cols);

    return KSO_NONE;
}


/*** Elements ***/

static KS_TFUNC(C, button_label) { 
    cnk_ctx self;
    ks_str label;
    KS_ARGS("self:* label:*", &self, kscnkt_ctx, &label, kst_str);

    /* TODO: how should we support image buttons? through another function, or different types? */

    int res = nk_button_label(self->ctx, label->data);

    /* Return whether the button was pressed */
    return KSO_BOOL(res != 0);
}

static KS_TFUNC(C, image) { 
    cnk_ctx self;
    cnk_image image;
    KS_ARGS("self:* image:*", &self, kscnkt_ctx, &image, kscnkt_image);

    /* TODO: should there be any autoconversion? */

    nk_image(self->ctx, image->val);

    return KSO_NONE;
}


static KS_TFUNC(C, edit_string) {
    cnk_ctx self;
    ks_str cur;
    ks_cint max_len, flags = 0;
    KS_ARGS("self:* cur:* max_len:cint ?flags:cint", &self, kscnkt_ctx, &cur, kst_str, &max_len, &flags);

    /* Create a modifiable buffer */
    char* buf = ks_malloc(max_len + 1);
    memcpy(buf, cur->data, cur->len_b);
    buf[cur->len_b] = '\0';

    /* Call function internally */
    ks_cint newflags = nk_edit_string_zero_terminated(self->ctx, flags, buf, max_len, nk_filter_default);

    /* Construct an output string */
    ks_str res = ks_str_new(-1, buf);

    /* Free temporary resources */
    ks_free(buf);

    return (kso)res;
}

#endif




/* Export */

static ks_module mnx, mmm;

static ks_module get() {

    /* Initialize platform specifics */

#if defined(CNK_USE_XLIB)


#elif defined(CNK_USE_GLFW)

    ks_debug("ks", "Calling gl3wInit()...");

    int stat;
    // Initialize OpenGL extension wrangler
    if (stat = gl3wInit()) {
        if (stat == GL3W_ERROR_LIBRARY_OPEN) {
            return ks_throw(ks_type_InternalError, "Could not initialize gl3w! (gl3wInit() failed!, reason: GL3W_ERROR_LIBRARY_OPEN)");
        } else {
            //return ks_throw(ks_type_InternalError, "Could not initialize gl3w! (gl3wInit() failed!)");
        }
    }

    // attempt to find a supported Major/Minor version of OpenGL
    int i;
    for (i = 0; _try_GLvers[i][0] > 0; ++i) {
        int major = _try_GLvers[i][0], minor = _try_GLvers[i][1];
        // see if this version is supported
        ks_debug("ks", "Trying OpenGL %i.%i", major, minor);
        if (gl3wIsSupported(major, minor) == 0) {
            // set the version to the correct one
            _GLvers[0] = major, _GLvers[1] = minor;
            break;
        }
    }

    /* Ensure valid OpenGL */
    if (_GLvers[0] < 0) return ks_throw(ks_type_Error, "Failed to initialize OpenGL");

    /* GLFW */

    glfwSetErrorCallback(_GLFW_errorcb);
    ks_debug("ks", "Calling glfwInit()...");

    /* Initialize GLFW */
    if (!glfwInit()) {
        return ks_throw(ks_type_Error, "Failed to initialize GLFW");
    }
    ks_debug("ks", "Setting GLFW Window Hints......");

    /* Set the OpenGL version being used */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, _GLvers[0]);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, _GLvers[1]);

    /* Set to core profile only (maximum compatibility) */
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  #endif

#endif


#ifdef BUILD_CNK

    ks_str knx = ks_str_new(-1, "nx");
    mnx = ks_import(knx);
    if (!knx) return NULL;
    KS_DECREF(knx);
    ks_str kmm = ks_str_new(-1, "mm");
    mmm = ks_import(kmm);
    if (!kmm) return NULL;
    KS_DECREF(kmm);


    kscnke_window = ks_enum_make(M_NAME ".Window", KS_EIKV(
        {"NONE", 0},
        {"BORDER", NK_WINDOW_BORDER},
        {"MOVABLE", NK_WINDOW_MOVABLE},
        {"SCALABLE", NK_WINDOW_SCALABLE},
        {"CLOSABLE", NK_WINDOW_CLOSABLE},
        {"MINIMIZABLE", NK_WINDOW_MINIMIZABLE},
        {"NO_SCROLLBAR", NK_WINDOW_NO_SCROLLBAR},
        {"TITLE", NK_WINDOW_TITLE},
        {"SCROLL_AUTO_HIDE", NK_WINDOW_SCROLL_AUTO_HIDE},
        {"BACKGROUND", NK_WINDOW_BACKGROUND},
        {"NO_INPUT", NK_WINDOW_NO_INPUT},
    ));

    kscnke_symbol = ks_enum_make(M_NAME ".Symbol", KS_EIKV(
        {"NONE", NK_SYMBOL_NONE},
        {"X", NK_SYMBOL_X},
        {"UNDERSCORE", NK_SYMBOL_UNDERSCORE},
        {"CIRCLE_SOLID", NK_SYMBOL_CIRCLE_SOLID},
        {"CIRCLE_OUTLINE", NK_SYMBOL_CIRCLE_OUTLINE},
        {"RECT_SOLID", NK_SYMBOL_RECT_SOLID},
        {"RECT_OUTLINE", NK_SYMBOL_RECT_OUTLINE},
        {"TRIANGLE_UP", NK_SYMBOL_TRIANGLE_UP},
        {"TRIANGLE_DOWN", NK_SYMBOL_TRIANGLE_DOWN},
        {"TRIANGLE_LEFT", NK_SYMBOL_TRIANGLE_LEFT},
        {"TRIANGLE_RIGHT", NK_SYMBOL_TRIANGLE_RIGHT},
        {"PLUS", NK_SYMBOL_PLUS},
        {"MINUS", NK_SYMBOL_MINUS},
        {"MAX", NK_SYMBOL_MAX},
    ));
    kscnke_heading = ks_enum_make(M_NAME ".Heading", KS_EIKV(
        {"UP", NK_UP},
        {"RIGHT", NK_RIGHT},
        {"DOWN", NK_DOWN},
        {"LEFT", NK_LEFT},
    ));

    kscnke_key = ks_enum_make(M_NAME ".Key", KS_EIKV(
        {"NONE", NK_KEY_NONE},
        {"SHIFT", NK_KEY_SHIFT},
        {"CTRL", NK_KEY_CTRL},
        {"DEL", NK_KEY_DEL},
        {"ENTER", NK_KEY_ENTER},
        {"TAB", NK_KEY_TAB},
        {"BACKSPACE", NK_KEY_BACKSPACE},
        {"COPY", NK_KEY_COPY},
        {"CUT", NK_KEY_CUT},
        {"PASTE", NK_KEY_PASTE},
        {"UP", NK_KEY_UP},
        {"DOWN", NK_KEY_DOWN},
        {"LEFT", NK_KEY_LEFT},
        {"RIGHT", NK_KEY_RIGHT},
        {"TEXT_INSERT_MODE", NK_KEY_TEXT_INSERT_MODE},
        {"TEXT_REPLACE_MODE", NK_KEY_TEXT_REPLACE_MODE},
        {"TEXT_RESET_MODE", NK_KEY_TEXT_RESET_MODE},
        {"TEXT_LINE_START", NK_KEY_TEXT_LINE_START},
        {"TEXT_LINE_END", NK_KEY_TEXT_LINE_END},
        {"TEXT_START", NK_KEY_TEXT_START},
        {"TEXT_END", NK_KEY_TEXT_END},
        {"TEXT_UNDO", NK_KEY_TEXT_UNDO},
        {"TEXT_REDO", NK_KEY_TEXT_REDO},
        {"TEXT_SELECT_ALL", NK_KEY_TEXT_SELECT_ALL},
        {"TEXT_WORD_LEFT", NK_KEY_TEXT_WORD_LEFT},
        {"TEXT_WORD_RIGHT", NK_KEY_TEXT_WORD_RIGHT},
        {"SCROLL_START", NK_KEY_SCROLL_START},
        {"SCROLL_END", NK_KEY_SCROLL_END},
        {"SCROLL_DOWN", NK_KEY_SCROLL_DOWN},
        {"SCROLL_UP", NK_KEY_SCROLL_UP},
        {"MAX", NK_KEY_MAX},
        
    ));

    kscnke_button = ks_enum_make(M_NAME ".Button", KS_EIKV(
        {"LEFT", NK_BUTTON_LEFT},
        {"MIDDLE", NK_BUTTON_MIDDLE},
        {"RIGHT", NK_BUTTON_RIGHT},
        {"DOUBLE", NK_BUTTON_DOUBLE},
        {"MAX", NK_BUTTON_MAX},
    ));

    kscnke_edit = ks_enum_make(M_NAME ".Edit", KS_EIKV(
        {"DEFAULT", NK_EDIT_DEFAULT},
        {"READ_ONLY", NK_EDIT_READ_ONLY},
        {"AUTO_SELECT", NK_EDIT_AUTO_SELECT},
        {"SIG_ENTER", NK_EDIT_SIG_ENTER},
        {"ALLOW_TAB", NK_EDIT_ALLOW_TAB},
        {"NO_CURSOR", NK_EDIT_NO_CURSOR},
        {"SELECTABLE", NK_EDIT_SELECTABLE},
        {"CLIPBOARD", NK_EDIT_CLIPBOARD},
        {"CTRL_ENTER_NEWLINE", NK_EDIT_CTRL_ENTER_NEWLINE},
        {"NO_HORIZONTAL_SCROLL", NK_EDIT_NO_HORIZONTAL_SCROLL},
        {"ALWAYS_INSERT_MODE", NK_EDIT_ALWAYS_INSERT_MODE},
        {"MULTILINE", NK_EDIT_MULTILINE},
        {"GOTO_END_ON_ACTIVATE", NK_EDIT_GOTO_END_ON_ACTIVATE},
        {"SIMPLE", NK_EDIT_SIMPLE},
        {"FIELD", NK_EDIT_FIELD},
        {"BOX", NK_EDIT_BOX},
        {"EDITOR", NK_EDIT_EDITOR},
    ));

    _ksinit(kscnkt_ctx, kst_object, TC_NAME, sizeof(struct cnk_ctx_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(C_free_, TC_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(C_new_, TC_NAME ".__new(tp, name, w, h)", "")},
        {"__next",                 ksf_wrap(C_next_, TC_NAME ".__next(self)", "")},
        {"__getattr",              ksf_wrap(C_getattr_, TC_NAME ".__getattr(self, attr)", "")},
        {"__setattr",              ksf_wrap(C_setattr_, TC_NAME ".__setattr(self, attr, val)", "")},
    

        {"isfocused",              ksf_wrap(C_isfocused_, TC_NAME ".isfocused(self)", "Returns whether or not the current window has focus")},
        {"ishovered",              ksf_wrap(C_ishovered_, TC_NAME ".ishovered(self, name)", "Returns whether or not the current window is being hovered over")},
        {"iscollapsed",            ksf_wrap(C_iscollapsed_, TC_NAME ".iscollapsed(self, name)", "Returns whether or not the window 'name' is collapsed")},
        {"isclosed",               ksf_wrap(C_isclosed_, TC_NAME ".isclosed(self, name)", "Returns whether or not the window 'name' is closed")},
        {"ishidden",               ksf_wrap(C_ishidden_, TC_NAME ".ishidden(self, name)", "Returns whether or not the window 'name' is hidden")},
        {"isactive",               ksf_wrap(C_isactive_, TC_NAME ".isactive(self, name)", "Returns whether or not the window 'name' is active")},
        {"isanyactive",            ksf_wrap(C_isanyactive_, TC_NAME ".isanyactive(self)", "Returns whether any window or widget or item is active")},

        {"set_bounds",             ksf_wrap(C_set_bounds_, TC_NAME ".set_bounds(self, name, x, y, w, h)", "Sets the bounding rectangle for the window 'name'")},
        {"set_pos",                ksf_wrap(C_set_pos_, TC_NAME ".set_pos(self, name, x, y)", "Sets the position for the window 'name'")},
        {"set_size",               ksf_wrap(C_set_size_, TC_NAME ".set_size(self, name, w, h)", "Sets the size for the window 'name'")},
        {"set_scroll",             ksf_wrap(C_set_scroll_, TC_NAME ".set_scroll(self, name, x, y)", "Sets the scroll offset (horizontal and vertical) for the window 'name'")},

        {"focus",                  ksf_wrap(C_focus_, TC_NAME ".focus(self, name)", "Focus on the window 'name'")},
        {"close",                  ksf_wrap(C_close_, TC_NAME ".close(self, name)", "Close the window 'name'")},
        {"minimize",               ksf_wrap(C_minimize_, TC_NAME ".minimize(self, name)", "Minimizes the window 'name'")},
        {"maximize",               ksf_wrap(C_maximize_, TC_NAME ".maximize(self, name)", "Maximizes the window 'name'")},
        {"show",                   ksf_wrap(C_show_, TC_NAME ".show(self, name)", "Shows the window 'name'")},
        {"hide",                   ksf_wrap(C_hide_, TC_NAME ".hide(self, name)", "Hides the window 'name'")},

        {"begin",                  ksf_wrap(C_begin_, TC_NAME ".begin(self, x, y, w, h, flags=nuklear.Window.NONE)", "Create a new window, with bounding rectangle of (x, y, w, h), with optional flags")},
        {"end",                    ksf_wrap(C_end_, TC_NAME ".end(self)", "End the last window created with 'begin()'")},
        

        {"layout_row_static",      ksf_wrap(C_layout_row_static_, TC_NAME ".layout_row_static(self, height, width, cols=1)", "Sets the current row layout")},
        {"layout_row_dynamic",     ksf_wrap(C_layout_row_dynamic_, TC_NAME ".layout_row_dynamic(self, height, cols=1)", "Sets the current row layout")},
        
        {"button_label",           ksf_wrap(C_button_label_, TC_NAME ".button(self, label)", "Draws a button with a given label")},
        {"image",                  ksf_wrap(C_image_, TC_NAME ".image(self, image)", "Draws an image")},
        
        {"edit_text",              ksf_wrap(C_edit_string_, TC_NAME ".edit_text(self, cur, max_len, flags=nuklear.Edit.NONE)", "Draws a string/text editor, and returns the new text created")},

    ));

    _ksinit(kscnkt_image, kst_object, TT_NAME, sizeof(struct cnk_image_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TT_free_, TT_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(TT_new_, TT_NAME ".__new(tp, img)", "")},
    ));


    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'mm' - multimedia module\n\n    This module implements common media operations", KS_IKV(
        /* Types */
        {"Context",                (kso)kscnkt_ctx},
        {"Image",                  (kso)kscnkt_image},

        /* Enums */
        {"Window",                 (kso)kscnke_window},
        {"Symbol",                 (kso)kscnke_symbol},
        {"Key",                    (kso)kscnke_key},
        {"Button",                 (kso)kscnke_button},
        {"Edit",                   (kso)kscnke_edit},

        /* Functions */

    ));

    return res;
#else
    KS_THROW(kst_ImportError, "Failed to import 'nuklear': Was not compiled with Xlib or GLFW, which is required");
    return NULL;
#endif
}

/* Loader function */
KS_CEXT_DECL(get);

