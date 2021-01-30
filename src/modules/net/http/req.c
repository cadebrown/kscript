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

static KS_TFUNC(T, init) {
    ksnet_http_req self;
    KS_ARGS("self:*", &self, ksnet_httpt_req);

    return KSO_NONE;
}

/* Export */

static struct ks_type_s tp;
ks_type ksnet_httpt_req = &tp;

void _ksi_net_http_req() {
    _ksinit(ksnet_httpt_req, kst_object, T_NAME, sizeof(struct ksnet_http_req_s), -1, "HTTP Request", KS_IKV(
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self)", "")},

    ));
}
