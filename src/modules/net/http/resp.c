/* resp.c - implementation of the 'net.http.Response' type
 *
 *
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>

#define T_NAME "net.http.Response"

/* Constants/Definitions/Utilities */

ksnet_http_resp ksnet_http_resp_new(ks_type tp, ks_str httpv, int status_code, ks_dict headers, ks_bytes body) {
    ksnet_http_resp self = KSO_NEW(ksnet_http_resp, tp);

    self->status_code = status_code;

    KS_INCREF(httpv);
    self->httpv = httpv;
    KS_INCREF(headers);
    self->headers = headers;
    if (!body) body = ks_bytes_new(0, NULL);
    else KS_INCREF(body);
    self->body = body;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksnet_http_resp self;
    KS_ARGS("self:*", &self, ksnet_httpt_resp);

    KS_DECREF(self->headers);
    KS_DECREF(self->httpv);
    KS_DECREF(self->body);

    return KSO_NONE;
}

static KS_TFUNC(T, init) {
    ksnet_http_resp self;
    KS_ARGS("self:*", &self, ksnet_httpt_resp);

    return KSO_NONE;
}

static KS_TFUNC(T, repr) {
    ksnet_http_resp self;
    KS_ARGS("self:*", &self, ksnet_httpt_resp);

    return (kso)ks_fmt("<'%T' code=%i>", self, self->status_code);
}

static KS_TFUNC(T, bytes) {
    ksnet_http_resp self;
    KS_ARGS("self:*", &self, ksnet_httpt_resp);

    ksio_BytesIO bio = ksio_BytesIO_new();
    ksio_add(bio, "%S %i %s" KS_CRLF, self->httpv, self->status_code, "OK");

    ks_ssize_t i;
    for (i = 0; i < self->headers->len_ents; ++i) {
        struct ks_dict_ent* ent = &self->headers->ents[i];
        if (ent->key) {
            ksio_add(bio, "%S: %S" KS_CRLF, ent->key, ent->val);
        }
    }
    ksio_add(bio, KS_CRLF);
    ksio_addbuf(bio, self->body->len_b, self->body->data);

    return (kso)ksio_BytesIO_getf(bio);
}


/* Export */

static struct ks_type_s tp;
ks_type ksnet_httpt_resp = &tp;


void _ksi_net_http_resp() {
    _ksinit(ksnet_httpt_resp, kst_object, T_NAME, sizeof(struct ksnet_http_resp_s), -1, "HTTP Response", KS_IKV(
        {"__free",                   ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                   ksf_wrap(T_init_, T_NAME ".__init(self)", "")},
        {"__bytes",                  ksf_wrap(T_bytes_, T_NAME ".__bytes(self)", "")},

        {"__repr",                   ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},
        {"__str",                    ksf_wrap(T_repr_, T_NAME ".__str(self)", "")},

    ));
}

