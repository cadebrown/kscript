/* SocketIO.c - implementation of 'SocketIO' for the 'net' module in kscript
 *
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>

#define T_NAME "net.SocketIO"

/* Constants/Definitions/Utilities */

/* Internal translation: k2u (kscript 2 unix) and u2k (unix 2 kscript)  */

/* Case macro */
#define C_(_c, _r) case _c: \
    *y = _r; \
    return true; \

#define E_(_c, _r) case _c: \
    KS_THROW(kst_Error, "Unsupported on this platform: %s (platform did not support %s)", #_c, #_r); \
    return false;

static bool k2u_fk(ksnet_fk x, int* y) {
    switch (x) {
#ifdef KS_HAVE_AF_INET
    C_(KSNET_FK_INET4, AF_INET)
#else
    E_(KSNET_FK_INET4, AF_INET)
#endif
#ifdef KS_HAVE_AF_INET6
    C_(KSNET_FK_INET6, AF_INET6)
#else
    E_(KSNET_FK_INET6, AF_INET6)
#endif
#ifdef KS_HAVE_AF_BLUETOOTH
    C_(KSNET_FK_BT, AF_BLUETOOTH)
#else
    E_(KSNET_FK_BT, AF_BLUETOOTH)
#endif
#ifdef KS_HAVE_AF_PACKET
    C_(KSNET_FK_PACKET, AF_PACKET)
#else
    E_(KSNET_FK_PACKET, AF_PACKET)
#endif
    }

    KS_THROW(kst_Error, "Unknown family kind: %i", (int)x);
    return false;
}
static bool k2u_sk(ksnet_sk x, int* y) {
    switch (x) {
#ifdef KS_HAVE_SOCK_RAW
    C_(KSNET_SK_RAW, SOCK_RAW)
#else
    E_(KSNET_SK_RAW, SOCK_RAW)
#endif
#ifdef KS_HAVE_SOCK_STREAM
    C_(KSNET_SK_TCP, SOCK_STREAM)
#else
    E_(KSNET_SK_TCP, SOCK_STREAM)
#endif
#ifdef KS_HAVE_SOCK_DGRAM
    C_(KSNET_SK_UDP, SOCK_DGRAM)
#else
    E_(KSNET_SK_UDP, SOCK_DGRAM)
#endif
#ifdef KS_HAVE_SOCK_PACKET
    C_(KSNET_SK_PACKET, SOCK_PACKET)
#else
    E_(KSNET_SK_PACKET, SOCK_PACKET)
#endif
#ifdef KS_HAVE_SOCK_SEQPACKET
    C_(KSNET_SK_PACKET_SEQ, SOCK_SEQPACKET)
#else
    E_(KSNET_SK_PACKET_SEQ, SOCK_SEQPACKET)
#endif
    }

    KS_THROW(kst_Error, "Unknown socket kind: %i", (int)x);
    return false;
}
static bool k2u_pk(ksnet_pk x, int* y) {
    switch (x) {
    C_(KSNET_PK_AUTO, 0)
    /*
    C_(KSNET_PK_BT_L2CAP, BTPROTO_L2CAP)
    C_(KSNET_PK_BT_RFCOMM, BTPROTO_RFCOMM)*/
    }

    KS_THROW(kst_Error, "Unknown protocol kind: %i", (int)x);
    return false;
}


ksnet_SocketIO ksnet_SocketIO_wrap(int fd, ksnet_fk fk, ksnet_sk sk, ksnet_pk pk, bool is_bound, bool is_listening) {
    ksnet_SocketIO self = KSO_NEW(ksnet_SocketIO, ksnett_SocketIO);

    self->fk = fk;
    self->sk = sk;
    self->pk = pk;

    self->src = ks_fmt("[socket:%i]", fd);
    self->mode = ks_str_new(-1, "rb+");
    self->mr = true;
    self->mw = true;
    self->mb = true;

    /* Copy settings */
    self->is_bound = is_bound;
    self->is_listening = is_listening;

    /* Copy socket descriptor */
    self->fd = fd;

    /* Set socket options */
    int opt = 1;

    /* Reuse addresses */
    if (setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0) {
        KS_DECREF(self);
        KS_THROW(kst_IOError, "Failed to set socket option (SO_REUSEADDR): %s", strerror(errno));
        return NULL;
    }

    /* Reuse port */
    #ifdef SO_REUSEPORT
    if(setsockopt(self->fd, SOL_SOCKET, SO_REUSEPORT, (void*)&opt, sizeof(opt)) < 0) {
        KS_DECREF(self);
        KS_THROW(kst_IOError, "Failed to set socket option (SO_REUSEPORT): %s", strerror(errno));
        return NULL;
    }
    #endif

    return self;
}

/* C-API Interface */
ksnet_SocketIO ksnet_SocketIO_new(ks_type tp, ksnet_fk fk, ksnet_sk sk, ksnet_pk pk) {
    /* Find out unix specific types*/
    int ufk, usk, upk;
    if (!k2u_fk(fk, &ufk) || !k2u_sk(sk, &usk) || !k2u_pk(pk, &upk)) {
        return NULL;
    }

    /* Create socket descriptor */
    int fd = socket(ufk, usk, upk);
    if (fd < 0) {
        KS_THROW(kst_IOError, "Failed to create socket: %s", strerror(errno));
        return NULL;
    }

    int opt = 1;
    /* Reuse addresses */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0) {
        KS_THROW(kst_IOError, "Failed to set socket option (%i): %s", SO_REUSEADDR, strerror(errno));
        return NULL;
    }

    /* Reuse port */
    #ifdef SO_REUSEPORT
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void*)&opt, sizeof(opt)) < 0) {
        KS_THROW(kst_IOError, "Failed to set socket option (%i): %s", SO_REUSEPORT, strerror(errno));
        return NULL;
    }
    #endif

    /* Now, wrap it */
    return ksnet_SocketIO_wrap(fd, fk, sk, pk, false, false);
}

bool ksnet_SocketIO_bind(ksnet_SocketIO self, kso addr) {
    int ufk;
    if (!k2u_fk(self->fk, &ufk)) {
        return false;
    }

    switch (self->fk)
    {
    case KSNET_FK_INET4:
        if (!kso_issub(addr->type, kst_tuple)) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but got '%T' object", addr);
            return false;
        }
        ks_tuple taddr = (ks_tuple)addr;
        if (taddr->len != 2) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but got tuple with wrong number of items", addr);
            return false;
        }

        ks_str hostname = (ks_str)taddr->elems[0];
        if (hostname->type != kst_str) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but host was not 'str', was '%T' object", addr, hostname);
            return false;
        }
        ks_cint port;
        if (!kso_get_ci(taddr->elems[1], &port)) return false;

        /* Get server address */
        self->addr.sin_family = ufk;
        self->addr.sin_port = htons(port);

        if (ks_str_eq_c(hostname, "localhost", 9)|| ks_str_eq_c(hostname, "", 0)) {
            self->addr.sin_addr.s_addr = INADDR_ANY;
        } else if (inet_pton(ufk, hostname->data, &self->addr.sin_addr) <= 0) {
            KS_THROW(kst_IOError, "Failed to resolve address: %R", hostname);
            return false;
        }

        /* Connect to address */
        KS_GIL_UNLOCK();
        if (bind(self->fd, (struct sockaddr *)&self->addr, sizeof(self->addr)) < 0)  { 
            KS_GIL_LOCK();
            KS_THROW(kst_IOError, "Failed to bind socket: %s", strerror(errno));
            return false;
        } 
        KS_GIL_LOCK();

        self->is_bound = true;
        break;
    
    default:
        KS_THROW(kst_Error, "Address families of kind '%i' not supported yet", (int)self->fk);
        break;
    }

    return true;
}

bool ksnet_SocketIO_connect(ksnet_SocketIO self, kso addr) {
    int ufk;
    if (!k2u_fk(self->fk, &ufk)) {
        return false;
    }

    switch (self->fk)
    {
    case KSNET_FK_INET4:
        if (!kso_issub(addr->type, kst_tuple)) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but got '%T' object", addr);
            return false;
        }
        ks_tuple taddr = (ks_tuple)addr;
        if (taddr->len != 2) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but got tuple with wrong number of items", addr);
            return false;
        }

        ks_str hostname = (ks_str)taddr->elems[0];
        if (hostname->type != kst_str) {
            KS_THROW(kst_ValError, "For 'INET4' address, expected '(host, port)', but host was not 'str', was '%T' object", addr, hostname);
            return false;
        }
        ks_cint port;
        if (!kso_get_ci(taddr->elems[1], &port)) return false;

        /* Get server address */
        struct sockaddr_in serv_addr;
        KS_GIL_UNLOCK();
        struct hostent* server = gethostbyname(hostname->data);
        KS_GIL_LOCK();
        if (!server) {
            KS_THROW(kst_IOError, "Failed to resolve address %R: %s", hostname, strerror(errno));
            return false;
        }

        /* Set from the host entry */
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = ufk;
        serv_addr.sin_port = htons((uint16_t)port);
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        /* Connect to address */
        if (connect(self->fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  { 
            KS_THROW(kst_IOError, "Failed to connect socket: %s", strerror(errno));
            return false;
        } 

        break;
    
    default:
        KS_THROW(kst_Error, "Address families of kind '%i' not supported yet", (int)self->fk);
        break;
    }

    return true;
}

bool ksnet_SocketIO_listen(ksnet_SocketIO self, int num) {
    if (!self->is_bound) {
        KS_THROW(kst_IOError, "Failed to listen on socket before it is bound");
        return false;
    }

    KS_GIL_UNLOCK();
    if (listen(self->fd, num) < 0) { 
        KS_GIL_LOCK();
        KS_THROW(kst_IOError, "Failed to listen on socket: %s", strerror(errno));
        return false;
    }
    KS_GIL_LOCK();
    
    self->is_listening = true;
    return true;
}

bool ksnet_SocketIO_accept(ksnet_SocketIO self, ksnet_SocketIO* client_socket, ks_str* client_addr) {
    if (!self->is_bound || !self->is_listening) {
        KS_THROW(kst_IOError, "Failed to accept on socket before it is bound and listening");
        return false;
    }

    /* Find out unix specific types*/
    int ufk, usk, upk;
    if (!k2u_fk(self->fk, &ufk) || !k2u_sk(self->sk, &usk) || !k2u_pk(self->pk, &upk)) {
        return false;
    }

    /* Accept connection */
    int sockfd_conn;
    struct sockaddr_in addr_conn;
    socklen_t addr_conn_len = sizeof(addr_conn);
    KS_GIL_UNLOCK();
    if ((sockfd_conn = accept(self->fd, (struct sockaddr*)&addr_conn, &addr_conn_len)) < 0) {
        KS_GIL_LOCK();
        KS_THROW(kst_IOError, "Failed to accept connection: %s", strerror(errno));
        return false;
    }
    KS_GIL_LOCK();

    /* Address name buffer */    
    char addr[KSNET_ADDR_MAX];
    KS_GIL_UNLOCK();
    if (!inet_ntop(addr_conn.sin_family, &addr_conn.sin_addr, addr, KSNET_ADDR_MAX)) {
        KS_GIL_LOCK();
        KS_THROW(kst_IOError, "Failed to get name for socket connection: %s", strerror(errno));
        return false;
    }
    KS_GIL_LOCK();

    /* Wrap the socket and return it */
    ksnet_SocketIO res = ksnet_SocketIO_wrap(sockfd_conn, self->fk, self->sk, self->pk, true, false);
    if (!res) {
        return false;
    }

    *client_socket = res;
    *client_addr = ks_str_new(-1, addr);
    return true;
}


ks_str ksnet_SocketIO_name(ksnet_SocketIO self) {
    if (!self->is_bound) {
        KS_THROW(kst_IOError, "Failed to get name for socket that is not bound");
        return false;
    }

    /* Address name buffer */    
    char addr[KSNET_ADDR_MAX];

    int ufk;
    if (!k2u_fk(self->fk, &ufk)) return NULL;
    if (!inet_ntop(ufk, &self->addr.sin_addr, addr, KSNET_ADDR_MAX)) {
        KS_THROW(kst_IOError, "Failed to get name for socket: %s", strerror(errno));
        return false;
    }

    return ks_str_new(-1, addr);
}

bool ksnet_SocketIO_port(ksnet_SocketIO self, int* out) {
    if (!self->is_bound) {
        KS_THROW(kst_IOError, "Failed to get port for socket that is not bound");
        return false;
    }

    *out = (int)ntohs(self->addr.sin_port);
    return true;
}

    
/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_cint fk = KSNET_FK_INET4;
    ks_cint sk = KSNET_SK_TCP;
    ks_cint pk = KSNET_PK_AUTO;
    KS_ARGS("tp:* ?fk:cint ?sk:cint ?pk:cint", &tp, kst_type, &fk, &sk, &pk);

    return (kso)ksnet_SocketIO_new(tp, fk, sk, pk);
}

static KS_TFUNC(T, init) {
    int nargs;
    kso* args;
    KS_ARGS("*args", &nargs, &args);

    return KSO_NONE;
}

static KS_TFUNC(T, free) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    if (self->fd >= 0) {
        shutdown(self->fd, SHUT_WR);
        shutdown(self->fd, SHUT_RD);
        close(self->fd);
    }
    KS_NDECREF(self->src);
    KS_NDECREF(self->mode);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, getattr) {
    ksnet_SocketIO self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksnett_SocketIO, &attr, kst_str);

    if (ks_str_eq_c(attr, "_fd", 3)) {
        return (kso)ks_int_new(self->fd);
    } else if (ks_str_eq_c(attr, "name", 4)) {
        return (kso)ksnet_SocketIO_name(self);
    } else if (ks_str_eq_c(attr, "port", 4)) {
        int r;
        if (!ksnet_SocketIO_port(self, &r)) return NULL;
        return (kso)ks_int_new(r);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, close) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    if (self->fd >= 0) {
        shutdown(self->fd, SHUT_WR);
        shutdown(self->fd, SHUT_RD);
        close(self->fd);
    }
    self->fd = -1;

    return KSO_NONE;
}

static KS_TFUNC(T, bind) {
    ksnet_SocketIO self;
    kso addr;
    KS_ARGS("self:* addr", &self, ksnett_SocketIO, &addr);

    if (!ksnet_SocketIO_bind(self, addr)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(T, connect) {
    ksnet_SocketIO self;
    kso addr;
    KS_ARGS("self:* addr", &self, ksnett_SocketIO, &addr);

    if (!ksnet_SocketIO_connect(self, addr)) return NULL;

    return KSO_NONE;
}
static KS_TFUNC(T, listen) {
    ksnet_SocketIO self;
    ks_cint num = 16;
    KS_ARGS("self:* ?num:cint", &self, ksnett_SocketIO, &num);

    if (!ksnet_SocketIO_listen(self, num)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(T, accept) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    ksnet_SocketIO client;
    ks_str name;
    if (!ksnet_SocketIO_accept(self, &client, &name)) return NULL;

    return (kso)ks_tuple_newn(2, (kso[]) {
        (kso)client,
        (kso)name
    });
}

/* Export */

static struct ks_type_s tp;
ks_type ksnett_SocketIO = &tp;

void _ksi_net_SocketIO() {

    _ksinit(ksnett_SocketIO, ksiot_RawIO, T_NAME, sizeof(struct ksnet_SocketIO_s), -1, "Represents a stream-like interface to a socket", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, fk=net.FK.INET4, sk=net.SK.TCP, pk=net.PK.AUTO)", "Create a new 'SocketIO' with the given parameters")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(*args)", "")},
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

        {"close",                  ksf_wrap(T_close_, T_NAME ".close(self)", "")},

        {"bind",                   ksf_wrap(T_bind_, T_NAME ".bind(self, addr)", "Bind the socket to the given address (the format depends on the type of socket)")},
        {"connect",                ksf_wrap(T_connect_, T_NAME ".connect(self, addr)", "Connect to the given address (the format depends on the type of socket)")},
        {"listen",                 ksf_wrap(T_listen_, T_NAME ".listen(self, num=16)", "Begin listening for connections, up to 'num' before refusing more")},
        {"accept",                 ksf_wrap(T_accept_, T_NAME ".accept(self)", "Accept a new connection, retuning a tuple of '(sock, name)' for the new connection")},

    ));

}

