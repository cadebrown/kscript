/* main.c - source code for the built-in 'net' module
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/net.h>

#define M_NAME "net"

/* Constants */

/* Utility Functions */

/* C API */

/* Export */

ks_type
    ksnete_sk = NULL,
    ksnete_fk = NULL,
    ksnete_pk = NULL
;


ks_module _ksi_net() {
    _ksi_net_SocketIO();

    ksnete_fk = ks_enum_make(M_NAME ".FK", KS_EIKV(
        {"INET4",                  KSNET_FK_INET4},
        {"INET6",                  KSNET_FK_INET6},
        {"BT",                     KSNET_FK_BT},
        {"PACKET",                 KSNET_FK_PACKET},
    ));

    ksnete_sk = ks_enum_make(M_NAME ".SK", KS_EIKV(
        {"RAW",                    KSNET_SK_RAW},
        {"TCP",                    KSNET_SK_TCP},
        {"UDP",                    KSNET_SK_UDP},
        {"PACKET",                 KSNET_SK_PACKET},
        {"PACKET_SEQ",             KSNET_SK_PACKET_SEQ},
    ));

    ksnete_pk = ks_enum_make(M_NAME ".PK", KS_EIKV(
        {"AUTO",                   KSNET_PK_AUTO},
        {"BT_L2CAP",               KSNET_PK_BT_L2CAP},
        {"BT_RFCOMM",              KSNET_PK_BT_RFCOMM},
    ));

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'net' - internet utilities\n\n    This module defines interacting with the internet", KS_IKV(

        /* Submodules */

       // {"http",                   (kso)_ksm_net_http()},


        /* Constants */

        {"FK",                     KS_NEWREF(ksnete_fk)},
        {"SK",                     KS_NEWREF(ksnete_sk)},
        {"PK",                     KS_NEWREF(ksnete_pk)},

        /* Types */

        {"SocketIO",               KS_NEWREF(ksnett_SocketIO)},

        /* Functions */

    ));

    return res;
}
