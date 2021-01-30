/* ks/net.h - header for the 'net' (networking module) module
 * 
 * Also includes the submodule 'net.http'
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNET_H__
#define KSNET_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif


/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif


#if defined(WIN32)

  /* Windows socket API */
  #include <winsock2.h>
  #include <Ws2tcpip.h>

#else

  /* Linux/Unix socket API 
   * TODO: detect different configurations?
   */
  #ifdef KS_HAVE_SYS_SOCKET_H
   #include <sys/socket.h>
  #endif
  #ifdef KS_HAVE_ARPA_INET_H
   #include <arpa/inet.h>
  #endif
  #ifdef KS_HAVE_NETDB_H
   #include <netdb.h>
  #endif
  #ifdef KS_HAVE_NETINET_IN_H
   #include <netinet/in.h>
  #endif

#endif


/** Constants **/

/* Maximum address length */
#define KSNET_ADDR_MAX 4096


/* Types of addresses (aka address families) */
typedef enum {
    KSNET_FK_NONE                  = 0,

    /* IPv4 Style Addresses
     *
     * addr: (host, port)
     * 
     * AKA: AF_INET
     */
    KSNET_FK_INET4                 = 1,

    /* IPv6 Style Addresses
     *
     * addr: (host, port, flowinfo, scopeid)
     *
     * AKA: AF_INET6
     */
    KSNET_FK_INET6                 = 2,

    /* Bluetooth Style Addresses
     *
     * AKA: AF_BLUETOOTH
     */
    KSNET_FK_BT                    = 3,


} ksnet_fk;

/* Types of sockets */
typedef enum {
    KSNET_SK_NONE                  = 0,

    /* Raw stream
     *
     * AKA: SOCK_RAW
     */
    KSNET_SK_RAW                   = 1,

    /* TCP Socket
     *
     * AKA: SOCK_STREAM
     */
    KSNET_SK_TCP                   = 2,

    /* UDP Socket
     * 
     * AKA: SOCK_DGRAM
     */
    KSNET_SK_UDP                   = 3,

    /* Packet Stream
     *
     * AKA: SOCK_PACKET
     */
    KSNET_SK_PACKET                = 4,

    /* Sequential Packet Stream
     *
     * AKA: SOCK_SEQPACKET
     */
    KSNET_SK_PACKET_SEQ            = 5,

} ksnet_sk;

/* Types of protocol */
typedef enum {

    /* Automatic Protocol
     */
    KSNET_PK_AUTO                  = 0,

    /* Bluetooth Protocol
     *
     * AKA: BTPROTO_L2CAP
     */
    KSNET_PK_BT_L2CAP              = 1,

    /* Bluetooth Protocol
     *
     * AKA: BTPROTO_RFCOMM
     */
    KSNET_PK_BT_RFCOMM             = 2,

} ksnet_pk;

/** Types **/

/* net.SocketIO - describes a networking socket, which can communicate over a network (or locally)
 *
 * Is a derived type of 'io.BaseIO'
 * 
 */
typedef struct ksnet_SocketIO_s {
    KSIO_RAWIO_BASE

    /* State */
    bool is_bound, is_listening;

    /* Socket, family, and protocol the socket is using */
    ksnet_sk sk;
    ksnet_fk fk;
    ksnet_pk pk;

    /* Address of the socket, if applicable */
    struct sockaddr_in addr;

}* ksnet_SocketIO;

/* Functions */

/* Create a new 'net.SocketIO' from the given kinds of family, socket, and protocol
 */
KS_API ksnet_SocketIO ksnet_SocketIO_new(ks_type tp, ksnet_fk fk, ksnet_sk sk, ksnet_pk pk);

/* Binds socket to a given address
 * Format of 'addr' depends on the family of the socket
 */
KS_API bool ksnet_SocketIO_bind(ksnet_SocketIO self, kso addr);

/* Connect 'self' to 'addr', as a client socket
 * Format of 'addr' depends on the family of the socket
 */
KS_API bool ksnet_SocketIO_connect(ksnet_SocketIO self, kso addr);

/* Begin listening for up to 'num' connections (at which point connections will automatically be refused)
 */
KS_API bool ksnet_SocketIO_listen(ksnet_SocketIO self, int num);

/* Accept a new connection from a socket, blocking until one was made
 */
KS_API bool ksnet_SocketIO_accept(ksnet_SocketIO self, ksnet_SocketIO* client_socket, ks_str* client_addr);


/* Get the hostname as a string
 */
KS_API ks_str ksnet_SocketIO_name(ksnet_SocketIO self);

/* Get the socket port
 */
KS_API bool ksnet_Socket_port(ksnet_SocketIO self, int* out);


/* Types */
KS_API_DATA ks_type
    ksnett_SocketIO
;



/** Submodule: net.http **/

/* net.http.Request - Represents a request sent to an HTTP(s) server
 *
 */
typedef struct ksnet_http_req_s {
    KSO_BASE

    /* The HTTP method being requested
     *
     * Should be one of:
     *  - 'GET'
     *  - 'HEAD'
     *  - 'POST'
     *  - 'PUT'
     *  - 'DELETE'
     *  - 'CONNECT'
     *  - 'OPTIONS'
     *  - 'TRACE'
     */
    ks_str method;

    /* Uniform Resource Identifier (i.e. target being requested)
     */
    ks_str uri;

    /* Protocol version
     * Almost always 'HTTP/1.1' 
     */
    ks_str httpv;

    /* Key-value headers
     * SEE: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers
     */
    ks_dict headers;

    /* Body sent along with the request */
    ks_bytes body;

}* ksnet_http_req;

/* 'net.http.Response' - represents an HTTP response sent back from a server
 *
 * SEE: https://www.tutorialspoint.com/http/http_responses.htm
 */
typedef struct ksnet_http_resp_s {
    KSO_BASE

    /* Version of HTTP protocol
     * Almost always 'HTTP/1.1'
     */
    ks_str httpv;

    /* Status code
     * SEE: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
     */
    int status_code;

    /* Key-value headers 
     * SEE: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers
     */
    ks_dict headers;

    /* Raw bytes being delivered */
    ks_bytes body;

}* ksnet_http_resp;


/* 'net.http.Server' - represents an HTTP server
 *
 * This is just the base class that any specific application should inherit from, which
 *   allows any number of attributes to be stored, and requires a socket (in attribute '.sock')
 *   to actually sent and receive messages on
 */
typedef struct ksnet_http_server_s {
    KSO_BASE

    /* __attr__ */
    ks_dict attr;
    
}* ksnet_http_server;


/* Functions */

/* Create a new 'net.http.Request'
 */
KS_API ksnet_http_req ksnet_http_req_new(ks_type tp, ks_str method, ks_str uri, ks_str httpv, ks_dict headers, ks_bytes body);

/* Create a new 'net.http.Response'
 */
KS_API ksnet_http_resp ksnet_http_resp_new(ks_type tp, ks_str httpv, int status_code, ks_dict headers, ks_bytes body);

/* Create a new HTTP server from a given address
 */
KS_API ksnet_http_server ksnet_http_server_new(ks_type tp, kso addr);

/* Serve forever on the current thread
 */
KS_API bool ksnet_http_server_serve_forever(ksnet_http_server self);

/* Attempt to parse a request and return it from a socket
 */
KS_API ksnet_http_req ksnet_http_get_request(ksio_BaseIO sock);



/* Types */
KS_API_DATA ks_type
    ksnet_httpt_req,
    ksnet_httpt_server,
    ksnet_httpt_resp
;




#endif /* KSNET_H__ */
