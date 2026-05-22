#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "types.h"

/* Address families (Linux/POSIX) */
#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_LOCAL        AF_UNIX
#define AF_INET         2
#define AF_AX25         3
#define AF_IPX          4
#define AF_APPLETALK    5
#define AF_NETROM       6
#define AF_BRIDGE       7
#define AF_ATMPVC       8
#define AF_X25          9
#define AF_INET6        10
#define AF_ROSE         11
#define AF_DECnet       12
#define AF_NETBEUI      13
#define AF_SECURITY     14
#define AF_KEY          15
#define AF_NETLINK      16
#define AF_PACKET       17
#define AF_ASH          18
#define AF_ECONET       19
#define AF_ATMSVC       20
#define AF_RDS          21
#define AF_SNA          22
#define AF_IRDA         23
#define AF_PPPOX        24
#define AF_WANPIPE      25
#define AF_LLC          26
#define AF_IB           27
#define AF_MPLS         28
#define AF_CAN          29
#define AF_TIPC         30
#define AF_BLUETOOTH    31
#define AF_IUCV         32
#define AF_RXRPC        33
#define AF_ISDN         34
#define AF_PHONET       35
#define AF_IEEE802154   36
#define AF_CAIF         37
#define AF_ALG          38
#define AF_VSOCK        40
#define AF_KCM          41
#define AF_QIPCRTR      42
#define AF_SMC          43
#define AF_XDP          44
#define AF_MAX          45

/* Socket types */
#define SOCK_STREAM      1
#define SOCK_DGRAM       2
#define SOCK_RAW         3
#define SOCK_RDM         4
#define SOCK_SEQPACKET   5
#define SOCK_DCCP        6
#define SOCK_PACKET      10
#define SOCK_CLOEXEC     (1 << 19)
#define SOCK_NONBLOCK    (1 << 20)
#define SOCK_TYPE_MASK   0xFF

/* IP protocol numbers */
#define IPPROTO_IP       0
#define IPPROTO_ICMP     1
#define IPPROTO_IGMP     2
#define IPPROTO_IPIP     4
#define IPPROTO_TCP      6
#define IPPROTO_EGP      8
#define IPPROTO_PUP      12
#define IPPROTO_UDP      17
#define IPPROTO_IDP      22
#define IPPROTO_TP       29
#define IPPROTO_DCCP     33
#define IPPROTO_IPV6     41
#define IPPROTO_ROUTING  43
#define IPPROTO_FRAGMENT 44
#define IPPROTO_GRE      47
#define IPPROTO_ESP      50
#define IPPROTO_AH       51
#define IPPROTO_ICMPV6   58
#define IPPROTO_NONE     59
#define IPPROTO_DSTOPTS  60
#define IPPROTO_MH       135
#define IPPROTO_SCTP     132
#define IPPROTO_RAW      255

/* Ethernet protocol IDs (network byte order constants) */
#define ETH_P_LOOP       0x0060
#define ETH_P_PUP        0x0200
#define ETH_P_PUPAT      0x0201
#define ETH_P_IP         0x0800
#define ETH_P_X25        0x0805
#define ETH_P_ARP        0x0806
#define ETH_P_BPQ        0x08FF
#define ETH_P_IEEEPUP    0x0A00
#define ETH_P_IEEEPUPAT  0x0A01
#define ETH_P_VLAN       0x8100
#define ETH_P_IPV6       0x86DD
#define ETH_P_PAUSE      0x8808
#define ETH_P_SLOW       0x8809
#define ETH_P_WCCP       0x883E
#define ETH_P_MPLS_UC    0x8847
#define ETH_P_MPLS_MC    0x8848
#define ETH_P_ATMMPOA    0x884C
#define ETH_P_PPP_DISC   0x8863
#define ETH_P_PPP_SES    0x8864
#define ETH_P_LINK_CTL   0x886C
#define ETH_P_ATMFATE    0x8884
#define ETH_P_PAE        0x888E
#define ETH_P_AOE        0x88A2
#define ETH_P_8021Q      0x8100
#define ETH_P_802_3      0x0001
#define ETH_P_AX25       0x0002
#define ETH_P_ALL        0x0003
#define ETH_P_802_2      0x0004
#define ETH_P_SNAP       0x0005
#define ETH_P_DDCMP      0x0006
#define ETH_P_WAN_PPP    0x0007
#define ETH_P_PPP_MP     0x0008
#define ETH_P_LOCALTALK  0x0009
#define ETH_P_CAN        0x000C
#define ETH_P_CANFD      0x000D
#define ETH_P_PPPTALK    0x0010
#define ETH_P_TR_802_2   0x0011
#define ETH_P_MOBITEX    0x0015
#define ETH_P_CONTROL    0x0016
#define ETH_P_IRDA       0x0017
#define ETH_P_ECONET     0x0018
#define ETH_P_HDLC       0x0019
#define ETH_P_ARCNET     0x001A
#define ETH_P_DSA        0x001B
#define ETH_P_TRAILER    0x001C
#define ETH_P_PHONET     0x00F5
#define ETH_P_IEEE802154 0x00F6
#define ETH_P_CAIF       0x00F7
#define ETH_P_IP_DGRAM   0x0800

/* Network interface types */
#define NETIF_TYPE_OTHER     0
#define NETIF_TYPE_ETHERNET  1
#define NETIF_TYPE_LOOPBACK  2
#define NETIF_TYPE_BRIDGE    3
#define NETIF_TYPE_VLAN      4
#define NETIF_TYPE_BOND      5
#define NETIF_TYPE_TUN       6
#define NETIF_TYPE_TAP       7
#define NETIF_TYPE_WIRELESS  8
#define NETIF_TYPE_INFINIBAND 9

/* sockaddr_in layout helpers */
#define SOCKADDR_IN_FAMILY_INET  AF_INET
#define SOCKADDR_IN_SIZE           16

const char* net_af_name(u32 af);
const char* net_sock_type_name(u32 type);
const char* net_ipproto_name(u32 proto);
u32 net_sock_type_base(u32 type);

#endif /* NET_SOCKET_H */
