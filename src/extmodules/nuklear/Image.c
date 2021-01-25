/* Image.c - implementation of the 'nuklear.Image' type
 *
 * 
 * SEE: https://immediate-mode-ui.github.io/Nuklear/doc/nuklear.html
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nuklear.h>

#define T_NAME "nuklear.Image"



/* C-API */

ksnk_Image ksnk_Image_new(ks_type tp, nx_t img) {
    int w, h, d;

    /* RGBA data */
    nx_u8* data = NULL;
    nx_dtype dt = nxd_u8;

    if (img.rank == 2) {
        /* Black and white */
        h = img.shape[0];
        w = img.shape[1];
        d = 1;
        data = ks_malloc(dt->size * w * h * 4);
        
        if (!nx_fpcast(
            nx_make(
                img.data,
                img.dtype,
                3,
                (ks_size_t[]) { img.shape[0], img.shape[1], 3 },
                (ks_ssize_t[]) { img.strides[0], img.strides[1], 0 }
            ),
            nx_make(
                data,
                dt,
                3,
                (ks_size_t[]){ h, w, 3 },
                (ks_ssize_t[]){ 4 * w * dt->size, 4 * dt->size, dt->size }
            )
        )) {
            ks_free(data);
            return NULL;
        }

        /* Set alpha channel */
        if (!nx_fpcast(
            nx_make(
                (nx_F[]){ 1.0f },
                nxd_F,
                0,
                NULL,
                NULL
            ),
            nx_make(
                &data[3],
                dt,
                3,
                (ks_size_t[]){ h, w, 1 },
                (ks_ssize_t[]){ 4 * w * dt->size, 4 * dt->size, 1 * dt->size }
            )
        )) {
            ks_free(data);
            return NULL;
        }


    } else if (img.rank == 3) {
        /* Color */
        h = img.shape[0];
        w = img.shape[1];
        d = img.shape[2];
        if (d > 4) d = 4;

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

        data = ks_malloc(dt->size * w * h * 4);
        if (!nx_fpcast(
            img,
            nx_make(
                data,
                dt,
                3,
                (ks_size_t[]){ h, w, (d == 1 ? 3 : d) },
                (ks_ssize_t[]){ 4 * w * dt->size, 4 * dt->size, 1 * dt->size }
            )
        )) {
            ks_free(data);
            return NULL;
        }

        /* Set alpha channel */
        if (d < 4 && !nx_fpcast(
            nx_make(
                (nx_F[]){ 1.0 },
                nxd_F,
                0,
                NULL,
                NULL
            ),
            nx_make(
                &data[3],
                dt,
                3,
                (ks_size_t[]){ h, w, 1 },
                (ks_ssize_t[]){ 4 * w * dt->size, 4 * dt->size, 1 * dt->size }
            )
        )) {
            ks_free(data);
            return NULL;
        }


    } else {
        KS_THROW(kst_Error, "Images must be created from rank-2 or rank-3 arrays (not rank-%i)", img.rank);
        return NULL;
    }


    assert(data != NULL);


#if defined(KSNK_XLIB_GL3) || defined(KSNK_GLFW_GL3)

    /* Generate OpenGL texture */
    ksnk_Image self = KSO_NEW(ksnk_Image, tp);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); 


    ks_free(data);
    /* Construct a wrapper for the Nuklear image */
    self->val = nk_image_id((int)tex);

    return self;

#else
    KS_THROW(kst_Error, "Failed to create image: Not compiled with OpenGL");
    ks_free(data);
    return NULL;
#endif

}



/* Type Functions */

static KS_TFUNC(T, free) {
    ksnk_Image self;
    KS_ARGS("self:*", &self, ksnkt_Image);

#if defined(KSNK_XLIB_GL3)

    /* Free OpenGL texture */
    if (self->gltex >= 0) {
        GLuint tex = self->gltex;
        glDeleteTextures(1, &tex);
    }

#elif defined(KSNK_GLFW_GL3)

    /* Free OpenGL texture */
    if (self->gltex >= 0) {
        GLuint tex = self->gltex;
        glDeleteTextures(1, &tex);
    }
#endif

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso img;
    KS_ARGS("tp:* img", &tp, kst_type, &img);

    nx_t ar;
    kso rr;
    if (!nx_get(img, NULL, &ar, &rr)) {
        return NULL;
    }

    ksnk_Image self = ksnk_Image_new(tp, ar);
    KS_NDECREF(rr);
    return (kso)self;
}


/* Export */

static struct ks_type_s tp;
ks_type ksnkt_Image = &tp;

void _ksi_nk_Image() {
    _ksinit(ksnkt_Image, kst_object, T_NAME, sizeof(struct ksnk_Image_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, img)", "")},

    ));
}


