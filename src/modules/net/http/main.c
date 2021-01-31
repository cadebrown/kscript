/* main.c - source code for the built-in 'net.http' module
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>

#define M_NAME "net.http"

/* Constants */

/* Utility Functions */

/* C API */


ksnet_http_req ksnet_http_get_request(ksio_BaseIO sock) {
    /* Request parsed thus far */
    char* data = NULL;
    ks_ssize_t len_data = 0, _max_len_data = 0;

    /* Buffer size to read in per loop */
    ks_ssize_t bsz = 256;

    /* Length in bytes of the header, or negative before it has been found */
    ks_ssize_t len_header = -1;

    ks_ssize_t i;

    #define _IS_SPACE(_c) ((_c) == '\n' || (_c) == '\r' || (_c) == ' ' || (_c) == '\t')
    #define _IS_NEWLINE(_c) ((_c) == '\n')

    while (len_header < 0) {

        if (len_data + bsz > _max_len_data) {
            _max_len_data = ks_nextsize(_max_len_data, len_data + bsz);
            data = ks_realloc(data, _max_len_data);
        }

        /* Receive up to 'bsz' bytes from the socket */
        ks_ssize_t rsz = ksio_readb((ksio_BaseIO)sock, bsz, data + len_data);
        //ks_ssize_t rsz = ksnet_Socket_recv_raw(sock, bsz, data + len_data);
        if (rsz < 0) {
            ks_free(data);
            return NULL;
        }
        len_data += rsz;


        /* Now, see if we have two newlines in a row (signals end of header) */
        bool lnl = false; /* last was newline */
        for (i = len_data - rsz; i < len_data && len_header < 0; ++i) {
            if (data[i] == '\n') {
                if (lnl) len_header = i;
                else lnl = true;
            } else if (_IS_SPACE(data[i])) {
                /* doesn't affect newline status */
            } else {
                lnl = false;
            }
        }
    }
    if (len_header < 0) {
        ks_free(data);
        KS_THROW(kst_Error, "Failed to parse HTTP header");
        return NULL;
    }

    /* Remove trailing spaces */
    while (len_header > 0 && _IS_SPACE(data[len_header - 1])) len_header--;

    /* Parse fields:
     * <METHOD> <URI> <HTTPV>
     */
    ks_ssize_t fs = i = 0;
    while (i < len_header && !_IS_SPACE(data[i])) i++;
    ks_str method = ks_str_new(i - fs, data + fs);
    fs = ++i; /* skip ' ' */
    while (i < len_header && !_IS_SPACE(data[i])) i++;
    ks_str uri = ks_str_new(i - fs, data + fs);
    fs = ++i; /* skip ' ' */
    while (i < len_header && !_IS_SPACE(data[i])) i++;
    ks_str httpv = ks_str_new(i - fs, data + fs);


    /* Parse headers:
     * <KEY>: <VALUE>\n
     */
    ks_dict headers = ks_dict_new(NULL);
    while (i < len_header) {
        while (i < len_header && _IS_SPACE(data[i])) i++;

        fs = i;
        while (i < len_header && data[i] != ':') i++;
        ks_str e_key = ks_str_new(i - fs, data + fs);
        ++i;

        // parse value
        while (i < len_header && _IS_SPACE(data[i])) i++;
        fs = i;
        while (i < len_header && !(data[i] == '\r' || _IS_NEWLINE(data[i]))) i++;

        // delete trailing spaces
        //while (i > 0 && _IS_SPACE(data[i - 1])) i--;        

        ks_str e_val = ks_str_new(i - fs, data + fs);

        if (!ks_dict_set_h(headers, (kso)e_key, e_key->v_hash, (kso)e_val)) {
            KS_DECREF(e_key);
            KS_DECREF(e_val);
            KS_DECREF(headers);
            KS_DECREF(method);
            KS_DECREF(uri);
            KS_DECREF(httpv);
            ks_free(data);
            return NULL;
        }

        KS_DECREF(e_key);
        KS_DECREF(e_val);
    }

    /* TODO: parse body */
    ks_bytes body = ks_bytes_new(0, NULL);
    ksnet_http_req res = ksnet_http_req_new(ksnet_httpt_req, method, uri, httpv, headers, body);
    KS_DECREF(method);
    KS_DECREF(uri);
    KS_DECREF(httpv);
    KS_DECREF(headers);
    KS_DECREF(body);

    return res;
}


/* Module Functions */

static KS_TFUNC(M, uriencode) {
    ks_str text;
    KS_ARGS("text:*", &text, kst_str);

    ksio_StringIO sio = ksio_StringIO_new();

    struct ks_str_citer cit = ks_str_citer_make(text);

    char* hexdig = "0123456789ABCDEF";
    while (true) {
        ks_ucp c = ks_str_citer_next(&cit);
        if (!c) break;
        if (cit.err) break;

        if (c < 128) {
            /* ASCII */
            if (c == '!' || c == '#' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' || c == '+' || c == ',' || c == '/' || c == ':' || c == ';' || c == '=' || c == '?' || c == '@' || c == '[' || c == ']' || c == '%' || c == ' ' || c == '\t' || c == '\n') {
                ksio_add(sio, "%%%c%c", hexdig[c / 16], hexdig[c % 16]);
            } else {
                ksio_add(sio, "%c", c);
            }
        } else {
            /* UTF8 */
            unsigned char utf8[5];
            int n;
            KS_UCP_TO_UTF8(utf8, n, c);
            if (n > 0) {
                int i;
                for (i = 0; i < n; ++i) {
                    ksio_add(sio, "%%%c%c", hexdig[utf8[i] / 16], hexdig[utf8[i] % 16]);
                }
            }
        }
    }

    return (kso)ksio_StringIO_getf(sio);
}

static KS_TFUNC(M, uridecode) {
    ks_str text;
    KS_ARGS("text:*", &text, kst_str);

    ksio_StringIO sio = ksio_StringIO_new();

    struct ks_str_citer cit = ks_str_citer_make(text);

    while (true) {
        ks_ucp c = ks_str_citer_next(&cit);
        if (!c) break;
        if (cit.err) break;

        if (c == '%') {
            /* %XX */
            ks_ucp x0 = ks_str_citer_next(&cit);
            ks_ucp x1 = ks_str_citer_next(&cit);
        
            if (!x0 || !x1) {
                KS_THROW(kst_Error, "Expected URI-encoded text to have 2 hex digits after a '%%'");
                KS_DECREF(sio);
                return NULL;
            }

            int d0 = 0;
            if ('0' <= x0 && x0 <= '9') {
                d0 = x0 - '0';
            } else if ('a' <= x0 && x0 <= 'f') {
                d0 = x0 - 'a' + 10;
            } else if ('A' <= x0 && x0 <= 'F') {
                d0 = x0 - 'A' + 10;
            } else {
                KS_THROW(kst_Error, "Expected URI-encoded text to have 2 hex digits after a '%%'");
                KS_DECREF(sio);
                return NULL;
            }

            int d1 = 0;
            if ('0' <= x1 && x1 <= '9') {
                d1 = x1 - '0';
            } else if ('a' <= x1 && x1 <= 'f') {
                d1 = x1 - 'a' + 10;
            } else if ('A' <= x1 && x1 <= 'F') {
                d1 = x1 - 'A' + 10;
            } else {
                KS_THROW(kst_Error, "Expected URI-encoded text to have 2 hex digits after a '%%'");
                KS_DECREF(sio);
                return NULL;
            }

            int bv = d0 * 16 + d1;
            ksio_add(sio, "%c", bv);

        } else {
            ksio_add(sio, "%c", c);
        }
    }

    return (kso)ksio_StringIO_getf(sio);

}


/* Export */

static ks_module my_time = NULL;

ks_module _ksi_net_http() {
    ks_str tk = ks_str_new(-1, "time");
    my_time = ks_import(tk);
    assert(my_time != NULL);
    KS_DECREF(tk);

    _ksi_net_http_server();
    _ksi_net_http_req();
    _ksi_net_http_resp();
   
    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'net' - internet utilities\n\n    This module defines interacting with the internet", KS_IKV(

        /* Constants */

        /* Types */
        {"Server",                 KS_NEWREF(ksnet_httpt_server)},
        {"Request",                KS_NEWREF(ksnet_httpt_req)},
        {"Response",               KS_NEWREF(ksnet_httpt_resp)},

        /* Functions */
        {"uriencode",              ksf_wrap(M_uriencode_, M_NAME ".uriencode(text)", "Encodes text to a URI-encoded component")},
        {"uridecode",              ksf_wrap(M_uridecode_, M_NAME ".uridecode(text)", "Decodes a URI-encoded component")},

    ));

    return res;
}
