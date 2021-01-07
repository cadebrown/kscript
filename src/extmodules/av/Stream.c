/* av/Stream.c - 'av.Stream' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>
#include <ks/av.h>


#define T_NAME "av.Stream"

/* Internals */

/* C-API */


ksav_Stream ksav_getstream(ksav_IO self, int sidx) {
    ksav_Stream res = KSO_NEW(ksav_Stream, ksavt_Stream);

    KS_INCREF(self);
    res->of = self;

    res->sidx = sidx;

    return res;
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ksav_Stream self;
    KS_ARGS("self:*", &self, ksavt_Stream);

    KS_DECREF(self->of);

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksav_Stream self;
    KS_ARGS("self:*", &self, ksavt_Stream);

    return (kso)ks_fmt("%R.stream(%i)", self->of, self->sidx);
}

static KS_TFUNC(T, next) {
    ksav_Stream self;
    KS_ARGS("self:*", &self, ksavt_Stream);

    int stream;
    kso res = ksav_next(self->of, &stream, 1, &self->sidx);
    if (!res) return NULL;

    assert(self->sidx == stream);
    return res;
}

static KS_TFUNC(T, isaudio) {
    ksav_Stream self;
    ks_cint idx = 0;
    KS_ARGS("self:* ?idx:cint", &self, ksavt_Stream, &idx);

    if (idx < 0 || idx >= self->of->nstreams) {
        KS_THROW_INDEX(self, _args[1]);
        return NULL;
    }

    bool res;
    if (!ksav_isaudio(self->of, idx, &res)) return NULL;

    return KSO_BOOL(res);
}

static KS_TFUNC(T, isvideo) {
    ksav_Stream self;
    ks_cint idx = 0;
    KS_ARGS("self:* ?idx:cint", &self, ksavt_Stream, &idx);

    if (idx < 0 || idx >= self->of->nstreams) {
        KS_THROW_INDEX(self, _args[1]);
        return NULL;
    }

    bool res;
    if (!ksav_isvideo(self->of, idx, &res)) return NULL;

    return KSO_BOOL(res);
}


/* Export */

static struct ks_type_s tp;
ks_type ksavt_Stream = &tp;

void _ksi_av_Stream() {
    _ksinit(ksavt_Stream, kst_object, T_NAME, sizeof(struct ksav_Stream_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},


        {"isaudio",                ksf_wrap(T_isaudio_, T_NAME ".isaudio(self, idx=0)", "Tells whether the stream is an audio stream")},
        {"isvideo",                ksf_wrap(T_isvideo_, T_NAME ".isvideo(self, idx=0)", "Tells whether the stream is a video stream")},

    ));
}


