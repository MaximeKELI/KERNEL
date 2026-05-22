#include "net_ports.h"
#include "string.h"
#include "stdio.h"

typedef struct {
    u16 port;
    const char* name;
} port_service_t;

static const port_service_t well_known_ports[] = {
    {PORT_TCPMUX, "tcpmux"},
    {PORT_ECHO, "echo"},
    {PORT_DISCARD, "discard"},
    {PORT_FTP_DATA, "ftp-data"},
    {PORT_FTP, "ftp"},
    {PORT_SSH, "ssh"},
    {PORT_TELNET, "telnet"},
    {PORT_SMTP, "smtp"},
    {PORT_NAMESERVER, "domain"},
    {PORT_TFTP, "tftp"},
    {PORT_HTTP, "http"},
    {PORT_POP3, "pop3"},
    {PORT_NTP, "ntp"},
    {PORT_IMAP, "imap"},
    {PORT_SNMP, "snmp"},
    {PORT_BGP, "bgp"},
    {PORT_LDAP, "ldap"},
    {PORT_HTTPS, "https"},
    {PORT_SMB, "smb"},
    {PORT_SYSLOG, "syslog"},
    {PORT_SMTPS, "smtps"},
    {PORT_SUBMISSION, "submission"},
    {PORT_IMAPS, "imaps"},
    {PORT_POP3S, "pop3s"},
    {PORT_MQTT, "mqtt"},
    {PORT_MYSQL, "mysql"},
    {PORT_POSTGRES, "postgresql"},
    {PORT_REDIS, "redis"},
    {PORT_HTTP_ALT, "http-alt"},
    {PORT_HTTPS_ALT, "https-alt"},
    {0, NULL},
};

void net_ports_init(void) {
    printk("[Ports] %zu well-known services registered\n",
           sizeof(well_known_ports) / sizeof(well_known_ports[0]) - 1);
}

bool net_port_valid(u16 port) {
    return port >= NET_PORT_MIN && port <= NET_PORT_MAX;
}

net_port_type_t net_port_classify(u16 port) {
    if (!net_port_valid(port)) {
        return NET_PORT_TYPE_INVALID;
    }
    if (port <= NET_PORT_PRIVILEGED_MAX) {
        return NET_PORT_TYPE_PRIVILEGED;
    }
    if (port <= NET_PORT_USER_MAX) {
        return NET_PORT_TYPE_REGISTERED;
    }
    return NET_PORT_TYPE_DYNAMIC;
}

const char* net_port_service_name(u16 port) {
    for (size_t i = 0; well_known_ports[i].name; i++) {
        if (well_known_ports[i].port == port) {
            return well_known_ports[i].name;
        }
    }
    return NULL;
}

u16 net_port_by_name(const char* name) {
    if (!name) {
        return 0;
    }
    for (size_t i = 0; well_known_ports[i].name; i++) {
        if (strcmp(name, well_known_ports[i].name) == 0) {
            return well_known_ports[i].port;
        }
    }
    return 0;
}
