#include "net_socket.h"
#include "string.h"

u32 net_sock_type_base(u32 type) {
    return type & SOCK_TYPE_MASK;
}

const char* net_af_name(u32 af) {
    switch (af) {
    case AF_UNSPEC: return "AF_UNSPEC";
    case AF_UNIX: return "AF_UNIX";
    case AF_INET: return "AF_INET";
    case AF_INET6: return "AF_INET6";
    case AF_PACKET: return "AF_PACKET";
    case AF_NETLINK: return "AF_NETLINK";
    case AF_BRIDGE: return "AF_BRIDGE";
    case AF_CAN: return "AF_CAN";
    case AF_VSOCK: return "AF_VSOCK";
    case AF_XDP: return "AF_XDP";
    default: return "AF_?";
    }
}

const char* net_sock_type_name(u32 type) {
    switch (net_sock_type_base(type)) {
    case SOCK_STREAM: return "SOCK_STREAM";
    case SOCK_DGRAM: return "SOCK_DGRAM";
    case SOCK_RAW: return "SOCK_RAW";
    case SOCK_RDM: return "SOCK_RDM";
    case SOCK_SEQPACKET: return "SOCK_SEQPACKET";
    case SOCK_DCCP: return "SOCK_DCCP";
    case SOCK_PACKET: return "SOCK_PACKET";
    default: return "SOCK_?";
    }
}

const char* net_ipproto_name(u32 proto) {
    switch (proto) {
    case IPPROTO_IP: return "IP";
    case IPPROTO_ICMP: return "ICMP";
    case IPPROTO_TCP: return "TCP";
    case IPPROTO_UDP: return "UDP";
    case IPPROTO_IPV6: return "IPv6";
    case IPPROTO_GRE: return "GRE";
    case IPPROTO_SCTP: return "SCTP";
    case IPPROTO_RAW: return "RAW";
    default: return "?";
    }
}
