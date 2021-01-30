/* server.c - implementation of the 'net.http.Server' type
 *
 * This isn't an abstract base class, but can be subclassed for others
 *
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>
#include <ks/time.h> /* for timestamps */

#define T_NAME "net.http.Server"

/* Constants/Definitions/Utilities */

/* C-API Interface */

ksnet_http_server ksnet_http_server_new(ks_type tp, kso addr) {
    ksnet_SocketIO sock = ksnet_SocketIO_new(ksnett_SocketIO, KSNET_FK_INET4, KSNET_SK_TCP, KSNET_PK_AUTO);
    if (!sock) {
        return NULL;
    }

    if (!ksnet_SocketIO_bind(sock, addr)) {
        KS_DECREF(sock);
        return NULL;
    }

    if (!ksnet_SocketIO_listen(sock, 16)) {
        KS_DECREF(sock);
        return NULL;
    }

    ksnet_http_server self = KSO_NEW(ksnet_http_server, tp);
    ks_dict_merge_ikv(self->attr, KS_IKV(
        {"addr",                   KS_NEWREF(addr)},
        {"sock",                   (kso)sock},
    ));

    return self;
}

bool ksnet_http_server_serve_forever(ksnet_http_server self) {

    ks_str tk = ks_str_new(-1, "_handle");
    kso handle = kso_getattr((kso)self, tk);
    KS_DECREF(tk);
    if (!handle) return false;

    ksnet_SocketIO sock = (ksnet_SocketIO)ks_dict_get_c(self->attr, "sock");
    if (!sock) {
        KS_DECREF(handle);
        return false;
    }

    ks_debug("net.http", "Serving forever: %R", self);

    ksnet_SocketIO client = NULL;
    ks_str addr = NULL;

    while (true) {

        /* Accept connection */
        if (!ksnet_SocketIO_accept(sock, &client, &addr)) {
            KS_DECREF(sock);
            KS_DECREF(handle);
            return false;
        }
        ks_debug("net.http", "New connection: %R (%R)", addr, client);

        /* Parse request */
        ksnet_http_req req = ksnet_http_get_request((ksio_BaseIO)client);
        if (!req) {
            KS_DECREF(sock);
            KS_DECREF(handle);
            KS_DECREF(client);
            KS_DECREF(addr);  
            return false;
        }

        ks_debug("request", "%S %R %S (%R)", req->method, req->uri, req->httpv, req->headers);

        /* Create new thread to handle the request in */
        ks_tuple args = ks_tuple_new(3, (kso[]){ (kso)addr, (kso)client, (kso)req });
        ksos_thread th = ksos_thread_new(ksost_thread, NULL, handle, args);
        KS_DECREF(args);
        KS_DECREF(client);
        KS_DECREF(addr);    
        KS_DECREF(req);
        if (!th) {
            KS_DECREF(sock);
            KS_DECREF(handle);
            return NULL;
        }

        if (!ksos_thread_start(th)) {
            KS_DECREF(sock);
            KS_DECREF(handle);
            return NULL;
        }

        if (!ksos_thread_join(th)) {
            KS_DECREF(sock);
            KS_DECREF(handle);
            return NULL;
        }
    }

    /* Sucesss */
    KS_DECREF(handle);
    KS_DECREF(sock);
    return true;
}

/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    kso addr;
    KS_ARGS("tp:* addr", &tp, kst_type, &addr);

    return (kso)ksnet_http_server_new(tp, addr);
}

static KS_TFUNC(T, serve_forever) {
    ksnet_http_server self;
    KS_ARGS("self:*", &self, ksnet_httpt_server);

    if (!ksnet_http_server_serve_forever(self)) return NULL;

    return KSO_NONE;
}


static KS_TFUNC(T, _handle) {
    ksnet_http_server self;
    ks_str addr;
    ksnet_SocketIO sock;
    ksnet_http_req req;
    KS_ARGS("self:* addr:* sock:* req:*", &self, ksnet_httpt_server, &addr, kst_str, &sock, ksnett_SocketIO, &req, ksnet_httpt_req);
    ks_str tk = ks_str_new(-1, "handle");
    kso handle = kso_getattr((kso)self, tk);
    KS_DECREF(tk);
    if (!handle) return NULL;

    kso res = kso_call((kso)handle, 3, (kso[]){ (kso)addr, (kso)sock, (kso)req });
    KS_DECREF(handle);
    if (!res) {
        return NULL;
    }


    if (kso_issub(res->type, ksnet_httpt_resp)) {
        ks_debug("response", "%R (%R)", res, ((ksnet_http_resp)res)->headers);

        ks_bytes resp_tob = ks_bytes_newo(kst_bytes, res);
        if (!resp_tob) {
            KS_DECREF(res);
            return NULL;
        }

        /* Actually send bytes */
        if (!ksio_addbuf(sock, resp_tob->len_b, resp_tob->data)) {
            KS_DECREF(resp_tob);
            return NULL;
        }
        KS_DECREF(resp_tob);
    
    }

    KS_DECREF(res);

    return KSO_NONE;
}

static KS_TFUNC(T, handle) {
    ksnet_http_server self;
    ks_str addr;
    ksnet_SocketIO sock;
    ksnet_http_req req;
    KS_ARGS("self:* addr:* sock:* req:*", &self, ksnet_httpt_server, &addr, kst_str, &sock, ksnett_SocketIO, &req, ksnet_httpt_req);

    /* Generate response */
    ks_bytes resp_body = NULL;
    int resp_status = -1;
    ksos_path path = ksos_path_new(-1, req->uri->data, KSO_NONE);
    if (!path) return NULL;

    ks_str atime = kstime_format(KSTIME_FMT_LOCALE, NULL);
    if (!atime) {
        KS_DECREF(path);
        return NULL;
    }

    ks_dict resp_headers = ks_dict_new(KS_IKV(
        {"Date",               (kso)atime},
        {"Server",             (kso)ks_fmt("kscript/Server")},
        {"Last-Modified",      (kso)atime},
        {"Connection",         (kso)ks_fmt("Closed")},
        {"Content-Disposition",(kso)ks_fmt("inline")},
    ));


    /* Read relative path */
    ks_str fsrcp = ks_str_new(-1, req->uri->data + 1);
    ks_str f_src = ksio_readall(fsrcp);
    KS_DECREF(fsrcp);

    if (!f_src) {
        /* File not found */
        resp_status = 404;

    } else {
        /* File was found */
        resp_status = 200;
        resp_body = ks_bytes_new(f_src->len_b, f_src->data);
        ks_dict_merge_ikv(resp_headers, KS_IKV(
            {"Content-Length", (kso)ks_fmt("%i", (int)resp_body->len_b)},
            {"Content-Type",   (kso)ks_fmt("text/plain;charset=UTF-8")},
        ));
        KS_DECREF(f_src);
    }

    KS_DECREF(path);

    /* Now, convert to object and then put that into bytes */
    ksnet_http_resp resp = ksnet_http_resp_new(ksnet_httpt_resp, req->httpv, resp_status, resp_headers, resp_body);
    KS_DECREF(resp_headers);
    if (resp_body) KS_DECREF(resp_body);
    return (kso)resp;
}


/* Export */

static struct ks_type_s tp;
ks_type ksnet_httpt_server = &tp;


void _ksi_net_http_server() {
    _ksinit(ksnet_httpt_server, kst_object, T_NAME, sizeof(struct ksnet_http_server_s), offsetof(struct ksnet_http_server_s, attr), "HTTP Server", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, addr)", "Create a new HTTP server")},

        {"serve_forever",          ksf_wrap(T_serve_forever_, T_NAME ".serve_forever(self)", "Serve forever, stalling the current thread")},
        {"_handle",                ksf_wrap(T__handle_, T_NAME "._handle(self, addr, sock, req)", "Internal connection handler")},
        {"handle",                 ksf_wrap(T_handle_, T_NAME ".handle(self, addr, sock, req)", "Handles a connection")},
    ));
}

