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


#ifdef _WIN32

/* Windows Headers */
#ifdef KS_HAVE_WINSOCK2_H
 #include <winsock2.h>
#endif
#ifdef KS_HAVE_WS2TCPIP_H
 #include <Ws2tcpip.h>
#endif

#else

/* Unix (assume they're all unix-like) */
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
     * AKA: PF_BLUETOOTH
     */
    KSNET_FK_BT                    = 3,


} ksnet_family_kind;


typedef enum {
    KSNET_SK_NONE                  = 0,

    /* TCP Socket
     *
     * AKA: SOCK_STREAM
     */
    KSNET_SK_TCP                   = 1,

    /* UDP Socket
     * 
     * AKA: SOCK_DGRAM
     */
    KSNET_SK_UDP                   = 2,

    /* Packet Stream
     *
     * AKA: SOCK_PACKET
     */
    KSNET_SK_PACKET                = 3,

    /* Sequential Packet Stream
     *
     * AKA: SOCK_SEQPACKET
     */
    KSNET_SK_PACKET_SEQ            = 4,

} ksnet_socket_kind;


typedef enum {

    /* Automatic Protocol
     */
    KSNET_PK_AUTO                  = 0,

    /* Bluetooth Protocol
     *
     * AKA: BTPROTO_L2CAP
     */
    KSNET_PK_BT_L2CAP              = 1,

} ksnet_proto_kind;


/** Types **/

/* net.SocketIO - describes a networking socket, which can communicate over a network (or locally)
 *
 * Is a base of 'io.AnyIO'
 * 
 */
typedef struct ksnet_SocketIO_s {
    KSO_BASE

    /* State */
    bool is_bound, is_listening, is_connected;

    /* Socket, family, and protocol the socket is using */
    ksnet_socket_kind sk;
    ksnet_family_kind fk;
    ksnet_proto_kind  pk;

    /* Platform-specifics (TODO: detect specifics) */

    struct {

        /* Socket Descriptor */
        int sd;

        /* Addres of the socket (only valid when bound) */
        struct sockaddr_in sa_addr;

    } _unix;

}* ksnet_SocketIO;

/* Create a new 'net.SocketIO' from the given parameters
 */
KS_API ksnet_SocketIO ksnet_SocketIO_new(ks_type tp, ksnet_family_kind fk, ksnet_socket_kind sk, ksnet_proto_kind pk);

/* Binds socket to a given address
 * Format of 'addr' depends on the family of the socket
 */
KS_API ksnet_SocketIO ksnet_SocketIO_bind(ksnet_SocketIO self, kso addr);

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

/* Retrieve the address name of a socket
 */
KS_API ks_str ksnet_SocketIO_get_addr_name(ksnet_SocketIO self);

/* Retreive the port a socket is bound to
 */
KS_API bool ksnet_SocketIO_get_port(ksnet_SocketIO self, int* out);





/** Functions **/

/* Types */
KS_API extern ks_type
    ksnett_SocketIO
;

#endif /* KSNET_H__ */
