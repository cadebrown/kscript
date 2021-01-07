/* Context.c - implementation of the 'nuklear.Context' type
 *
 * 
 * SEE: https://immediate-mode-ui.github.io/Nuklear/doc/nuklear.html
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nuklear.h>

#define T_NAME "nuklear.Context"

/* Internals */


/* Maximum vertex buffer size */
static int ctx_max_verbuf = 512 * 1024;

/* Maximum element buffer size */
static int ctx_max_elebuf = 128 * 1024;

#if defined(KSNK_XLIB_GL3)


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

#endif

/* Type Functions */

static KS_TFUNC(T, free) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    /* Clean up Nuklear objects */

#ifdef KSNK_DO
    nk_free(self->ctx);
    nk_font_atlas_cleanup(self->atlas);
#endif

#if defined(KSNK_XLIB_GL3)

    /* Free Xlib stuffs */
    XFree(self->x.vis);

#elif defined(KSNK_GLFW_GL3)

    /* Destroy the GLFW window */
    glfwDestroyWindow(self->window);

#endif

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_str name;
    ks_cint w, h;
    KS_ARGS("tp:* name:* w:cint h:cint", &tp, kst_type, &name, kst_str, &w, &h);

    ksnk_Context self = KSO_NEW(ksnk_Context, tp);

    self->n_frames = 0;

    self->col_bk[0] = self->col_bk[1] = self->col_bk[2] = 0.1f;


#if defined(KSNK_XLIB_GL3)

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
        self->x.glx_ctx = glXCreateNewContext(self->x.display, self->x.fbc, GLX_RGBA_TYPE, 0, True);
    } else {
        GLint attr[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            None
        };
        self->x.glx_ctx = create_context(self->x.display, self->x.fbc, 0, True, attr);
        XSync(self->x.display, False);
        if (hadErr || !self->x.glx_ctx) {
            /* Could not create GL 3.0 context. Fallback to old 2.x context.
                * If a version below 3.0 is requested, implementations will
                * return the newest context version compatible with OpenGL
                * version less than version 3.0.*/
            attr[1] = 1; attr[3] = 0;
            hadErr = false;
            ks_info("ks", "[X11]: Failed to create OpenGL 3.0 context, using old style GLX context");
            self->x.glx_ctx = create_context(self->x.display, self->x.fbc, 0, True, attr);
        }
    }


    XSync(self->x.display, False);
    if (hadErr || !self->x.glx_ctx) {
        KS_THROW(kst_Error, "Failed to create an OpenGL context in Xlib");
        return NULL;
    }

    /* Make it the current context */
    glXMakeCurrent(self->x.display, self->x.window, self->x.glx_ctx);

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

#elif defined(KSNK_GLFW_GL3)

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




static KS_TFUNC(T, getattr) {
    ksnk_Context self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksnkt_Context, &attr, kst_str);

    if (ks_str_eq_c(attr, "size", 4)) {
        /* Return (w, h) tuple of the window size */
        int ww, wh;

#if defined(KSNK_XLIB_GL3)
        XGetWindowAttributes(self->x.display, self->x.window, &self->x.attr);
        ww = self->x.attr.width;
        wh = self->x.attr.height;
#elif defined(KSNK_GLFW_GL3)
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


static KS_TFUNC(T, setattr) {
    ksnk_Context self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, ksnkt_Context, &attr, kst_str, &val);

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
#if defined(KSNK_XLIB_GL3)
        XResizeWindow(self->x.display, self->x.window, w, h);
#elif defined(KSNK_GLFW_GL3)
        glfwSetWindowSize(self->window, w, h);
#endif

        return KSO_NONE;
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


/** Nuklear API **/


#ifdef KSNK_DO

/*** Iteration/Drawing ***/

/*** Drawing ***/

static KS_TFUNC(T, start_frame) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    bool rst = true;

#if defined(KSNK_XLIB_GL3)

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

#elif defined(KSNK_GLFW_GL3)

    glfwPollEvents();
    nk_glfw3_new_frame();
    if (glfwWindowShouldClose(self->window)) rst = false;

#endif

    return KSO_BOOL(rst);
}

static KS_TFUNC(T, end_frame) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    bool rst = true;

    /* Query the size */
    int dw, dh;
#if defined(KSNK_XLIB_GL3)
    XGetWindowAttributes(self->x.display, self->x.window, &self->x.attr);
    dw = self->x.attr.width;
    dh = self->x.attr.height;
#elif defined(KSNK_GLFW_GL3)
    glfwGetWindowSize(self->window, &dw, &dh);
#endif


    /* Shared OpenGL code */

    /* Render the whole window */
    glViewport(0, 0, dw, dh);

    /* Set background color */
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(self->col_bk[0], self->col_bk[1], self->col_bk[2], self->col_bk[3]);


#if defined(KSNK_XLIB_GL3)

    nk_x11_render(NK_ANTI_ALIASING_ON, ctx_max_verbuf, ctx_max_elebuf);
    glXSwapBuffers(self->x.display, self->x.window);

#elif defined(KSNK_GLFW_GL3)
    // use the Nuklear GLFW3 render helper (with our configuration of max memory)
    nk_glfw3_render(NK_ANTI_ALIASING_ON, ctx_max_verbuf, ctx_max_elebuf);

    // swap buffers
    glfwSwapBuffers(self->window);

    if (glfwWindowShouldClose(self->window)) rst = false;
#endif

    /* Return whether it should keep going */
    return KSO_BOOL(rst);
}


static KS_TFUNC(T, next) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    bool t;
    kso r;

    if (self->n_frames > 0) {
        r = T_end_frame_(1, (kso[]){ (kso)self });
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

    r = T_start_frame_(1, (kso[]){ (kso)self });
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

static KS_TFUNC(T, isfocused) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    int res = nk_window_has_focus(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, ishovered) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    int res = nk_window_is_hovered(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, iscollapsed) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    int res = nk_window_is_collapsed(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, isclosed) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    int res = nk_window_is_closed(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, ishidden) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    int res = nk_window_is_hidden(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, isactive) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    int res = nk_window_is_active(self->ctx, name->data);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, isanyactive) {
    ksnk_Context self;
    KS_ARGS("self:* name:*", &self, ksnkt_Context);

    int res = nk_item_is_any_active(self->ctx);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, set_bounds) {
    ksnk_Context self;
    ks_str name;
    ks_cfloat x, y, w, h;
    KS_ARGS("self:* name:* x:cfloat y:cfloat w:cfloat h:cfloat", &self, ksnkt_Context, &name, kst_str, &x, &y, &w, &h);

    nk_window_set_bounds(self->ctx, name->data, nk_rect(x, y, w, h));

    return KSO_NONE;
}

static KS_TFUNC(T, set_pos) {
    ksnk_Context self;
    ks_str name;
    ks_cfloat x, y;
    KS_ARGS("self:* name:* x:cfloat y:cfloat", &self, ksnkt_Context, &name, kst_str, &x, &y);

    nk_window_set_position(self->ctx, name->data, nk_vec2(x, y));

    return KSO_NONE;
}

static KS_TFUNC(T, set_size) {
    ksnk_Context self;
    ks_str name;
    ks_cfloat w, h;
    KS_ARGS("self:* name:* w:cfloat h:cfloat", &self, ksnkt_Context, &name, kst_str, &w, &h);

    nk_window_set_size(self->ctx, name->data, nk_vec2(w, h));

    return KSO_NONE;
}

static KS_TFUNC(T, set_scroll) {
    ksnk_Context self;
    ks_str name;
    ks_uint x, y;
    KS_ARGS("self:* name:* x:cint y:cint", &self, ksnkt_Context, &name, kst_str, &x, &y);

    nk_window_set_scroll(self->ctx, x, y);

    return KSO_NONE;
}

static KS_TFUNC(T, focus) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_set_focus(self->ctx, name->data);

    return KSO_NONE;
}

static KS_TFUNC(T, close) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_close(self->ctx, name->data);

    return KSO_NONE;
}

static KS_TFUNC(T, minimize) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_collapse(self->ctx, name->data, NK_MINIMIZED);

    return KSO_NONE;
}

static KS_TFUNC(T, maximize) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_collapse(self->ctx, name->data, NK_MAXIMIZED);

    return KSO_NONE;
}

static KS_TFUNC(T, show) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_show(self->ctx, name->data, NK_SHOWN);

    return KSO_NONE;
}

static KS_TFUNC(T, hide) {
    ksnk_Context self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, ksnkt_Context, &name, kst_str);

    nk_window_show(self->ctx, name->data, NK_HIDDEN);

    return KSO_NONE;
}


/*** Layout ***/

static KS_TFUNC(T, begin) {
    ksnk_Context self;
    ks_str title;
    ks_cfloat x, y, w, h;
    ks_cint flags;
    KS_ARGS("self:* title:* x:cfloat y:cfloat w:cfloat h:cfloat ?flags:cint", &self, ksnkt_Context, &title, kst_str, &x, &y, &w, &h, &flags);

    int res = nk_begin(self->ctx, title->data, nk_rect(x, y, w, h), flags);

    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, end) {
    ksnk_Context self;
    KS_ARGS("self:*", &self, ksnkt_Context);

    nk_end(self->ctx);
   
    return KSO_NONE;
}

static KS_TFUNC(T, layout_row_static) {
    ksnk_Context self;
    ks_cfloat height;
    ks_cint width, cols;
    KS_ARGS("self:* height:cfloat width:cint ?cols:cint", &self, ksnkt_Context, &height, &width, &cols);

    nk_layout_row_static(self->ctx, height, width, cols);

    return KSO_NONE;
}

static KS_TFUNC(T, layout_row_dynamic) {
    ksnk_Context self;
    ks_cfloat height;
    ks_cint cols;
    KS_ARGS("self:* height:cfloat ?cols:cint", &self, ksnkt_Context, &height, &cols);

    nk_layout_row_static(self->ctx, height, height, cols);

    return KSO_NONE;
}


/*** Elements ***/

static KS_TFUNC(T, button_label) { 
    ksnk_Context self;
    ks_str label;
    KS_ARGS("self:* label:*", &self, ksnkt_Context, &label, kst_str);

    /* TODO: how should we support image buttons? through another function, or different types? */

    int res = nk_button_label(self->ctx, label->data);

    /* Return whether the button was pressed */
    return KSO_BOOL(res != 0);
}

static KS_TFUNC(T, image) { 
    ksnk_Context self;
    ksnk_Image image;
    KS_ARGS("self:* image:*", &self, ksnkt_Context, &image, ksnkt_Image);

    /* TODO: should there be any autoconversion? */

    nk_image(self->ctx, image->val);

    return KSO_NONE;
}


static KS_TFUNC(T, edit_string) {
    ksnk_Context self;
    ks_str cur;
    ks_cint max_len, flags = 0;
    KS_ARGS("self:* cur:* max_len:cint ?flags:cint", &self, ksnkt_Context, &cur, kst_str, &max_len, &flags);

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

#endif /* KSNK_DO */


/* Export */

static struct ks_type_s tp;
ks_type ksnkt_Context = &tp;

void _ksi_nk_Context() {
    _ksinit(ksnkt_Context, kst_object, T_NAME, sizeof(struct ksnk_Context_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, name, w, h)", "")},

        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__setattr",              ksf_wrap(T_setattr_, T_NAME ".__setattr(self, attr, val)", "")},

#ifdef KSNK_DO

        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},


        {"isfocused",              ksf_wrap(T_isfocused_, T_NAME ".isfocused(self)", "Returns whether or not the current window has focus")},
        {"ishovered",              ksf_wrap(T_ishovered_, T_NAME ".ishovered(self, name)", "Returns whether or not the current window is being hovered over")},
        {"iscollapsed",            ksf_wrap(T_iscollapsed_, T_NAME ".iscollapsed(self, name)", "Returns whether or not the window 'name' is collapsed")},
        {"isclosed",               ksf_wrap(T_isclosed_, T_NAME ".isclosed(self, name)", "Returns whether or not the window 'name' is closed")},
        {"ishidden",               ksf_wrap(T_ishidden_, T_NAME ".ishidden(self, name)", "Returns whether or not the window 'name' is hidden")},
        {"isactive",               ksf_wrap(T_isactive_, T_NAME ".isactive(self, name)", "Returns whether or not the window 'name' is active")},
        {"isanyactive",            ksf_wrap(T_isanyactive_, T_NAME ".isanyactive(self)", "Returns whether any window or widget or item is active")},

        {"set_bounds",             ksf_wrap(T_set_bounds_, T_NAME ".set_bounds(self, name, x, y, w, h)", "Sets the bounding rectangle for the window 'name'")},
        {"set_pos",                ksf_wrap(T_set_pos_, T_NAME ".set_pos(self, name, x, y)", "Sets the position for the window 'name'")},
        {"set_size",               ksf_wrap(T_set_size_, T_NAME ".set_size(self, name, w, h)", "Sets the size for the window 'name'")},
        {"set_scroll",             ksf_wrap(T_set_scroll_, T_NAME ".set_scroll(self, name, x, y)", "Sets the scroll offset (horizontal and vertical) for the window 'name'")},

        {"focus",                  ksf_wrap(T_focus_, T_NAME ".focus(self, name)", "Focus on the window 'name'")},
        {"close",                  ksf_wrap(T_close_, T_NAME ".close(self, name)", "Close the window 'name'")},
        {"minimize",               ksf_wrap(T_minimize_, T_NAME ".minimize(self, name)", "Minimizes the window 'name'")},
        {"maximize",               ksf_wrap(T_maximize_, T_NAME ".maximize(self, name)", "Maximizes the window 'name'")},
        {"show",                   ksf_wrap(T_show_, T_NAME ".show(self, name)", "Shows the window 'name'")},
        {"hide",                   ksf_wrap(T_hide_, T_NAME ".hide(self, name)", "Hides the window 'name'")},

        {"begin",                  ksf_wrap(T_begin_, T_NAME ".begin(self, x, y, w, h, flags=nuklear.Window.NONE)", "Create a new window, with bounding rectangle of (x, y, w, h), with optional flags")},
        {"end",                    ksf_wrap(T_end_, T_NAME ".end(self)", "End the last window created with 'begin()'")},
        

        {"layout_row_static",      ksf_wrap(T_layout_row_static_, T_NAME ".layout_row_static(self, height, width, cols=1)", "Sets the current row layout")},
        {"layout_row_dynamic",     ksf_wrap(T_layout_row_dynamic_, T_NAME ".layout_row_dynamic(self, height, cols=1)", "Sets the current row layout")},
        
        {"button_label",           ksf_wrap(T_button_label_, T_NAME ".button(self, label)", "Draws a button with a given label")},
        {"image",                  ksf_wrap(T_image_, T_NAME ".image(self, image)", "Draws an image")},
        
        {"edit_text",              ksf_wrap(T_edit_string_, T_NAME ".edit_text(self, cur, max_len, flags=nuklear.Edit.NONE)", "Draws a string/text editor, and returns the new text created")},
#endif
    ));
}


