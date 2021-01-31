/* req.c - implementation of the 'net.http.Request' type
 *
 *
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>

#define T_NAME "net.http.Request"

/* Constants/Definitions/Utilities */

ksnet_http_req ksnet_http_req_new(ks_type tp, ks_str method, ks_str uri, ks_str httpv, ks_dict headers, ks_bytes body) {
    ksnet_http_req self= KSO_NEW(ksnet_http_req, tp);

    KS_INCREF(method);
    self->method = method;
    KS_INCREF(uri);
    self->uri = uri;
    KS_INCREF(httpv);
    self->httpv = httpv;
    KS_INCREF(headers);
    self->headers = headers;
    KS_INCREF(body);
    self->body = body;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksnet_http_req self;
    KS_ARGS("self:*", &self, ksnet_httpt_req);

    KS_NDECREF(self->method);
    KS_NDECREF(self->uri);
    KS_NDECREF(self->httpv);
    KS_NDECREF(self->body);
    KS_NDECREF(self->headers);

    KSO_DEL(self);
    return KSO_NONE;
}
static KS_TFUNC(T, new) {
    ks_type tp;
    ks_str method, uri, httpv;
    kso headers;
    kso body;
    KS_ARGS("tp:* method:* uri:* httpv:* headers body", &tp, kst_type, &method, kst_str, &uri, kst_str, &httpv, kst_str, &headers, &body);

    ks_dict dh = (ks_dict)kso_call((kso)kst_dict, 1, &headers);
    if (!dh) {
        return NULL;
    }

    ks_bytes db = ks_bytes_newo(kst_bytes, body);
    if (!db) {
        KS_DECREF(dh);
        return NULL;
    }

    ksnet_http_req res = ksnet_http_req_new(tp, method, uri, httpv, dh, db);
    KS_DECREF(dh);
    KS_DECREF(db);

    return (kso)res;
}

static KS_TFUNC(T, str) {
    ksnet_http_req self;
    KS_ARGS("self:*", &self, ksnet_httpt_req);

    return (kso)ks_fmt("<%T method=%R, uri=%R, httpv=%R, headers=%R, body=%R>", self, self->method, self->uri, self->httpv, self->headers, self->body);
}

static KS_TFUNC(T, getattr) {
    ksnet_http_req self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksnet_httpt_req, &attr, kst_str);

    if (ks_str_eq_c(attr, "method", 6)) {
        return KS_NEWREF(self->method);
    } else if (ks_str_eq_c(attr, "uri", 3)) {
        return KS_NEWREF(self->uri);
    } else if (ks_str_eq_c(attr, "httpv", 5)) {
        return KS_NEWREF(self->httpv);
    } else if (ks_str_eq_c(attr, "headers", 7)) {
        return KS_NEWREF(self->headers);
    } else if (ks_str_eq_c(attr, "body", 4)) {
        return KS_NEWREF(self->body);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

/* Export */

static struct ks_type_s tp;
ks_type ksnet_httpt_req = &tp;

void _ksi_net_http_req() {
    _ksinit(ksnet_httpt_req, kst_object, T_NAME, sizeof(struct ksnet_http_req_s), -1, "HTTP Request", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, method, uri, httpv, headers, body)", "")},

        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

    ));
}
