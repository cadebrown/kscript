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


static ksnet_SocketIO ksnet_SocketIO_wrap(int fd, int fk, int sk, int pk, bool is_bound, bool is_listening) {
    ksnet_SocketIO self = KSO_NEW(ksnet_SocketIO, ksnett_SocketIO);

    self->fk = fk;
    self->sk = sk;
    self->pk = pk;

    /* Copy settings */
    self->is_connected = true;
    self->is_bound = is_bound;
    self->is_listening = is_listening;

    /* Copy socket descriptor */
    self->_unix.sd = fd;
    if (self->_unix.sd < 0) {
        KS_DECREF(self);
        KS_THROW(kst_IOError, "Failed to wrap UNIX-style socket");
        return NULL;
    }

    /* Set socket options */
    int opt = 1;

    /* Reuse addresses */
    if (setsockopt(self->_unix.sd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0) {
        KS_DECREF(self);
        KS_THROW(kst_IOError, "Failed to set socket option (SO_REUSEADDR): %s", strerror(errno));
        return NULL;
    }

    /* Reuse port */
    #ifdef SO_REUSEPORT
    if(setsockopt(self->_unix.sd, SOL_SOCKET, SO_REUSEPORT, (void*)&opt, sizeof(opt)) < 0) {
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
    int usd = socket(ufk, usk, upk);
    if (usd < 0) {
        KS_THROW(kst_IOError, "Failed to create socket: %s", strerror(errno));
        return NULL;
    }

    int opt = 1;
    /* Reuse addresses */
    if (setsockopt(usd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0) {
        KS_THROW(kst_IOError, "Failed to set socket option (%i): %s", SO_REUSEADDR, strerror(errno));
        return NULL;
    }

    /* Reuse port */
    #ifdef SO_REUSEPORT
    if(setsockopt(usd, SOL_SOCKET, SO_REUSEPORT, (void*)&opt, sizeof(opt)) < 0) {
        KS_THROW(kst_IOError, "Failed to set socket option (%i): %s", SO_REUSEPORT, strerror(errno));
        return NULL;
    }
    #endif

    /* Now, wrap it */
    ksnet_SocketIO self = KSO_NEW(ksnet_SocketIO, ksnett_SocketIO);

    self->fk = fk;
    self->sk = sk;
    self->pk = pk;

    self->_unix.sd = usd;

    self->is_bound = self->is_connected = self->is_listening = false;

    return self;
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
        self->_unix.sa_addr.sin_family = ufk;
        self->_unix.sa_addr.sin_port = htons(port);

        if (ks_str_eq_c(hostname, "localhost", 9)|| ks_str_eq_c(hostname, "", 0)) {
            self->_unix.sa_addr.sin_addr.s_addr = INADDR_ANY;
        } else if (inet_pton(ufk, hostname->data, &self->_unix.sa_addr.sin_addr) <= 0) {
            KS_THROW(kst_IOError, "Failed to resolve address: %R", hostname);
            return false;
        }

        /* Connect to address */
        KS_GIL_UNLOCK();
        if (bind(self->_unix.sd, (struct sockaddr *)&self->_unix.sa_addr, sizeof(self->_unix.sa_addr)) < 0)  { 
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
        if (connect(self->_unix.sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  { 
            KS_THROW(kst_IOError, "Failed to connect socket: %s", strerror(errno));
            return false;
        } 

        self->is_connected = true;
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
    if (listen(self->_unix.sd, num) < 0) { 
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
    if ((sockfd_conn = accept(self->_unix.sd, (struct sockaddr*)&addr_conn, &addr_conn_len)) < 0) {
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
    if (!inet_ntop(ufk, &self->_unix.sa_addr.sin_addr, addr, KSNET_ADDR_MAX)) {
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

    *out = (int)ntohs(self->_unix.sa_addr.sin_port);
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

static KS_TFUNC(T, free) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    close(self->_unix.sd);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, getattr) {
    ksnet_SocketIO self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksnett_SocketIO, &attr, kst_str);

    if (ks_str_eq_c(attr, "_fd", 3)) {
        return (kso)ks_int_new(self->_unix.sd);
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

static KS_TFUNC(T, isopen) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    if (!self->is_connected) return KSO_FALSE;

    char c;
    ks_ssize_t x = recv(self->_unix.sd, &c, 1, MSG_PEEK);
    if (x > 0) {
        return KSO_TRUE;
    } else if (x == 0) {
        self->is_connected = false;
        return KSO_FALSE;
    } else {
        /* error */
        KS_THROW(kst_IOError, "Failed to detect whether socket was open: %s", strerror(errno));
        return NULL;
    }
}

static KS_TFUNC(T, rtype) {
    ksnet_SocketIO self;
    KS_ARGS("self:*", &self, ksnett_SocketIO);

    return KS_NEWREF(kst_bytes);
}

static KS_TFUNC(T, read) {
    ksnet_SocketIO self;
    ks_cint sz = KS_CINT_MAX;
    KS_ARGS("self:* ?sz:cint", &self, ksnett_SocketIO, &sz);

    /* Read bytes */
    ks_ssize_t bsz = KSIO_BUFSIZ, rsz = 0;
    void* dest = NULL;
    while (rsz < sz && self->is_connected) {
        dest = ks_realloc(dest, rsz + bsz);
        ks_ssize_t msz = sz - rsz;
        if (msz <= 0) break;
        if (msz > bsz) msz = bsz;
        ks_ssize_t csz = read(self->_unix.sd, ((char*)dest) + rsz, msz);
        if (csz < 0) {
            KS_THROW(kst_IOError, "Failed to read from socket: %s", strerror(errno));
            ks_free(dest);
            return NULL;

        } else if (csz == 0) {
            self->is_connected = false;
            break;
        } else {
            rsz += csz;
            if (csz < msz) {
                self->is_connected = false;
            }
        }
    }
    ks_bytes res = ks_bytes_new(rsz, dest);
    ks_free(dest);
    return (kso)res;
}

static KS_TFUNC(T, write) {
    ksnet_SocketIO self;
    kso msg;
    KS_ARGS("self:* msg", &self, ksnett_SocketIO, &msg);

    ks_bytes bio = ks_bytes_newo(kst_bytes, msg);
    if (!bio) return NULL;

    ks_ssize_t csz = write(self->_unix.sd, bio->data, bio->len_b);
    if (csz < 0) {
        KS_THROW(kst_IOError, "Failed to write to socket: %s", strerror(errno));
        KS_DECREF(bio);
        return NULL;
    } else if (csz == 0) {
        self->is_connected = false;
    }

    KS_DECREF(bio);
    return (kso)ks_int_new(csz);
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

    _ksinit(ksnett_SocketIO, ksiot_BaseIO, T_NAME, sizeof(struct ksnet_SocketIO_s), -1, "Represents a stream-like interface to a socket", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, fk=net.FK.INET4, sk=net.SK.TCP, pk=net.PK.AUTO)", "Create a new 'SocketIO' with the given parameters")},
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

        {"rtype",                  ksf_wrap(T_rtype_, T_NAME ".rtype(self)", "")},

        {"isopen",                 ksf_wrap(T_isopen_, T_NAME ".isopen(self)", "Calculates whether the socket is open")},

        {"read",                   ksf_wrap(T_read_, T_NAME ".read(self, sz=-1)", "Read a message from the socket")},
        {"write",                  ksf_wrap(T_write_, T_NAME ".write(self, msg)", "Writes a message to the socket")},


        {"bind",                   ksf_wrap(T_bind_, T_NAME ".bind(self, addr)", "Bind the socket to the given address (the format depends on the type of socket)")},
        {"connect",                ksf_wrap(T_connect_, T_NAME ".connect(self, addr)", "Connect to the given address (the format depends on the type of socket)")},
        {"listen",                 ksf_wrap(T_listen_, T_NAME ".listen(self, num=16)", "Begin listening for connections, up to 'num' before refusing more")},
        {"accept",                 ksf_wrap(T_accept_, T_NAME ".accept(self)", "Accept a new connection, retuning a tuple of '(sock, name)' for the new connection")},


/*
        {"bind",                   kso_func_new(T_bind_, T_NAME ".bind(self, addr)", "Bind the socket to the given address (which depends on what kind of socket it is)")},
        {"connect",                kso_func_new(T_connect_, T_NAME ".connect(self, addr)", "Connect the socket to the given address (which depends on what kind of socket it is)")},

        {"listen",                 kso_func_new(T_listen_, T_NAME ".listen(self, num)", "Begin listening\n\n    If 'num' is given, then connections will be rejected once 'num' connections are queued up")},
        {"accept",                 kso_func_new(T_accept_, T_NAME ".accept(self)", "Wait for a new request to come in, and return a tuple of '(socket, clientname)'")},

        {"send",                   kso_func_new(T_send_, T_NAME ".send(self, msg)", "Send a message over the socket")},
        {"recv",                   kso_func_new(T_recv_, T_NAME ".recv(self, msglen)", "Receive a message from the socket, with 'msglen' bytes")},
*/
    ));

}

