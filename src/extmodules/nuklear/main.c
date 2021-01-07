/* main.c - source code for the built-in 'nuklear' module
 *
 * 
 * SEE: https://immediate-mode-ui.github.io/Nuklear/doc/nuklear.html
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/cext.h>
#include <ks/nuklear.h>

#define M_NAME "nuklear"


/* Types */


/* Internals */

/* Versions of OpenGL to try to intiialize with */
static int I_glvers[][2] = {
    {3, 3},
    {-1, -1},
};

/* Major and minor versions used */
static int glMajor = -1, glMinor = -1;


#if defined(KSNK_XLIB_GL3)

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

#elif defined(KSNK_GLFW_GL3)

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



/* Module Functions */


/* Export */

ks_type 
    ksnke_Heading, 
    ksnke_Symbol, 
    ksnke_Window,
    ksnke_Key,
    ksnke_Button, 
    ksnke_Edit
;

static ks_module get() {

    /* Initialize platform specifics */

#if defined(KSNK_XLIB_GL3)

    ks_debug("nuklear", "Initializing: gl3w");


    int st;
    if ((st = gl3wInit()) != 0) {
        if (st == GL3W_ERROR_LIBRARY_OPEN) {
            KS_THROW(kst_Error, "Failed to initialize gl3w: gl3wInit() returned GL3W_ERROR_LIBRARY_OPEN");
            return NULL;
        } else {
            //KS_THROW(kst_Error, "Failed to initialize gl3w: gl3wInit() returned non-zero");
            //return NULL;
        }
    }


    /* Attempt to open different OpenGL versions */
    int i, found = 0;
    for (i = 0; I_glvers[i][0] > 0; ++i) {
        glMajor = I_glvers[i][0];
        glMinor = I_glvers[i][1];
        /* Check version of OpenGL */
        ks_debug("nuklear", "Trying OpenGL %i.%i", glMajor, glMinor);
        if (gl3wIsSupported(glMajor, glMinor) == 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        KS_THROW(kst_Error, "Failed to initialize OpenGL through gl3w");
        return NULL;
    }


#elif defined(KSNK_GLFW_GL3)

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


#ifdef KSNK_DO

    ks_str knx = ks_str_new(-1, "nx");
    ks_module mnx = ks_import(knx);
    if (!mnx) return NULL;
    KS_DECREF(knx);
    ks_str kav = ks_str_new(-1, "av");
    ks_module mav = ks_import(kav);
    if (!mav) return NULL;
    KS_DECREF(kav);



    _ksi_nk_Context();
    _ksi_nk_Image();

    ksnke_Window = ks_enum_make(M_NAME ".Window", KS_EIKV(
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

    ksnke_Symbol = ks_enum_make(M_NAME ".Symbol", KS_EIKV(
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
    ksnke_Heading = ks_enum_make(M_NAME ".Heading", KS_EIKV(
        {"UP", NK_UP},
        {"RIGHT", NK_RIGHT},
        {"DOWN", NK_DOWN},
        {"LEFT", NK_LEFT},
    ));

    ksnke_Key = ks_enum_make(M_NAME ".Key", KS_EIKV(
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

    ksnke_Button = ks_enum_make(M_NAME ".Button", KS_EIKV(
        {"LEFT", NK_BUTTON_LEFT},
        {"MIDDLE", NK_BUTTON_MIDDLE},
        {"RIGHT", NK_BUTTON_RIGHT},
        {"DOUBLE", NK_BUTTON_DOUBLE},
        {"MAX", NK_BUTTON_MAX},
    ));

    ksnke_Edit = ks_enum_make(M_NAME ".Edit", KS_EIKV(
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

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'mm' - multimedia module\n\n    This module implements common media operations", KS_IKV(
        /* Types */
        {"Context",                (kso)ksnkt_Context},
        {"Image",                  (kso)ksnkt_Image},

        /* Enums */
        {"Window",                 (kso)ksnke_Window},
        {"Symbol",                 (kso)ksnke_Symbol},
        {"Key",                    (kso)ksnke_Key},
        {"Button",                 (kso)ksnke_Button},
        {"Edit",                   (kso)ksnke_Edit},

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



