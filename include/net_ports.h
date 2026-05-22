#ifndef NET_PORTS_H
#define NET_PORTS_H

#include "types.h"

/* TCP/UDP port number ranges (IANA) */
#define NET_PORT_MIN              1
#define NET_PORT_MAX              65535
#define NET_PORT_PRIVILEGED_MAX   1023
#define NET_PORT_USER_MIN         1024
#define NET_PORT_USER_MAX         49151
#define NET_PORT_DYNAMIC_MIN      49152
#define NET_PORT_DYNAMIC_MAX      65535

/* Port classification */
typedef enum {
    NET_PORT_TYPE_INVALID = 0,
    NET_PORT_TYPE_PRIVILEGED,
    NET_PORT_TYPE_REGISTERED,
    NET_PORT_TYPE_DYNAMIC,
} net_port_type_t;

/* Well-known TCP/UDP ports (IANA common) */
#define PORT_TCPMUX      1
#define PORT_ECHO        7
#define PORT_DISCARD     9
#define PORT_DAYTIME     13
#define PORT_NETSTAT     15
#define PORT_QOTD        17
#define PORT_FTP_DATA    20
#define PORT_FTP         21
#define PORT_SSH         22
#define PORT_TELNET      23
#define PORT_SMTP        25
#define PORT_TIME        37
#define PORT_NAMESERVER  42
#define PORT_WHOIS       43
#define PORT_TFTP        69
#define PORT_GOPHER      70
#define PORT_FINGER      79
#define PORT_HTTP        80
#define PORT_KERBEROS    88
#define PORT_HOSTNAME    101
#define PORT_POP3        110
#define PORT_NTP         123
#define PORT_NETBIOS_NS  137
#define PORT_NETBIOS_DGM 138
#define PORT_NETBIOS_SSN 139
#define PORT_IMAP        143
#define PORT_SNMP        161
#define PORT_SNMP_TRAP   162
#define PORT_BGP         179
#define PORT_LDAP        389
#define PORT_HTTPS       443
#define PORT_SMB         445
#define PORT_SYSLOG      514
#define PORT_LPD         515
#define PORT_RTSP        554
#define PORT_SMTPS       465
#define PORT_SUBMISSION  587
#define PORT_IMAPS       993
#define PORT_POP3S       995
#define PORT_MQTT        1883
#define PORT_MQTTS       8883
#define PORT_MYSQL       3306
#define PORT_RDP         3389
#define PORT_POSTGRES    5432
#define PORT_VNC         5900
#define PORT_REDIS       6379
#define PORT_HTTP_ALT    8080
#define PORT_HTTPS_ALT   8443

void net_ports_init(void);
bool net_port_valid(u16 port);
net_port_type_t net_port_classify(u16 port);
const char* net_port_service_name(u16 port);
u16 net_port_by_name(const char* name);

#endif /* NET_PORTS_H */
