#include "network_info.h"
#include <arpa/inet.h>
#include <glib.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

char *get_network_info(void) {
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) return g_strdup("getifaddrs failed.");

    GString *out = g_string_new("Interface\tFamily\tAddress\tStatus\n");
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        int fam = ifa->ifa_addr->sa_family;
        if (fam != AF_INET && fam != AF_INET6) continue;

        char host[NI_MAXHOST];
        int len = (fam == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
        if (getnameinfo(ifa->ifa_addr, len, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
            g_string_append_printf(out, "%s\t%s\t%s\t%s\n",
                ifa->ifa_name,
                fam == AF_INET ? "IPv4" : "IPv6",
                host,
                (ifa->ifa_flags & IFF_UP) ? "UP" : "DOWN");
        }
    }
    freeifaddrs(ifaddr);
    return g_string_free(out, FALSE);
}
