#include "network_manager.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct PacketCapture {
    int fd;
    gboolean running;
    GThread *thread;
    PacketFilter filter;
    PacketCaptureCallback callback;
    gpointer user_data;
};

static gboolean read_uint64_file(const char *path, guint64 *value) {
    char *content = NULL;
    if (!g_file_get_contents(path, &content, NULL, NULL)) return FALSE;
    g_strstrip(content);
    *value = g_ascii_strtoull(content, NULL, 10);
    g_free(content);
    return TRUE;
}

GPtrArray *network_manager_list_interfaces(void) {
    GPtrArray *items = g_ptr_array_new_with_free_func(g_free);
    GDir *dir = g_dir_open("/sys/class/net", 0, NULL);
    if (!dir) return items;

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        g_ptr_array_add(items, g_strdup(name));
    }
    g_dir_close(dir);
    return items;
}

gboolean network_manager_read_traffic(const char *iface, TrafficStats *stats, char **error_message) {
    if (!iface || !*iface) {
        if (error_message) *error_message = g_strdup("Interface is required.");
        return FALSE;
    }

    char *rx_path = g_strdup_printf("/sys/class/net/%s/statistics/rx_bytes", iface);
    char *tx_path = g_strdup_printf("/sys/class/net/%s/statistics/tx_bytes", iface);
    gboolean ok = read_uint64_file(rx_path, &stats->rx_bytes) && read_uint64_file(tx_path, &stats->tx_bytes);

    if (!ok && error_message) {
        *error_message = g_strdup_printf("Cannot read traffic statistics for %s.", iface);
    }
    g_free(rx_path);
    g_free(tx_path);
    return ok;
}

char *network_manager_format_bytes(guint64 bytes) {
    const char *units[] = {"B", "KB", "MB", "GB"};
    double value = (double)bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        unit++;
    }
    return g_strdup_printf(unit == 0 ? "%.0f %s" : "%.2f %s", value, units[unit]);
}

char *network_manager_format_speed(double bytes_per_second) {
    guint64 rounded = bytes_per_second < 0 ? 0 : (guint64)bytes_per_second;
    char *size = network_manager_format_bytes(rounded);
    char *speed = g_strdup_printf("%s/s", size);
    g_free(size);
    return speed;
}

PacketFilter packet_filter_from_text(const char *text) {
    if (g_strcmp0(text, "TCP") == 0) return PACKET_FILTER_TCP;
    if (g_strcmp0(text, "UDP") == 0) return PACKET_FILTER_UDP;
    if (g_strcmp0(text, "ICMP") == 0) return PACKET_FILTER_ICMP;
    return PACKET_FILTER_ALL;
}

static gboolean packet_matches_filter(PacketFilter filter, const char *protocol) {
    return filter == PACKET_FILTER_ALL ||
           (filter == PACKET_FILTER_TCP && g_strcmp0(protocol, "TCP") == 0) ||
           (filter == PACKET_FILTER_UDP && g_strcmp0(protocol, "UDP") == 0) ||
           (filter == PACKET_FILTER_ICMP && g_strcmp0(protocol, "ICMP") == 0);
}

static void fill_time_text(char *buffer, gsize size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_now;
    localtime_r(&tv.tv_sec, &tm_now);
    strftime(buffer, size, "%H:%M:%S", &tm_now);
}

static gboolean parse_packet(const guint8 *buffer, ssize_t len, PacketInfo *packet) {
    if (len < (ssize_t)sizeof(struct ethhdr)) return FALSE;
    const struct ethhdr *eth = (const struct ethhdr *)buffer;
    guint16 eth_type = ntohs(eth->h_proto);

    memset(packet, 0, sizeof(*packet));
    fill_time_text(packet->time_text, sizeof(packet->time_text));
    packet->length = (guint)len;

    if (eth_type == ETH_P_IP) {
        if (len < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct iphdr))) return FALSE;
        const struct iphdr *ip = (const struct iphdr *)(buffer + sizeof(struct ethhdr));
        inet_ntop(AF_INET, &ip->saddr, packet->source_ip, sizeof(packet->source_ip));
        inet_ntop(AF_INET, &ip->daddr, packet->dest_ip, sizeof(packet->dest_ip));
        if (ip->protocol == IPPROTO_TCP) g_strlcpy(packet->protocol, "TCP", sizeof(packet->protocol));
        else if (ip->protocol == IPPROTO_UDP) g_strlcpy(packet->protocol, "UDP", sizeof(packet->protocol));
        else if (ip->protocol == IPPROTO_ICMP) g_strlcpy(packet->protocol, "ICMP", sizeof(packet->protocol));
        else g_strlcpy(packet->protocol, "OTHER", sizeof(packet->protocol));
        return TRUE;
    }

    if (eth_type == ETH_P_IPV6) {
        if (len < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct ip6_hdr))) return FALSE;
        const struct ip6_hdr *ip6 = (const struct ip6_hdr *)(buffer + sizeof(struct ethhdr));
        inet_ntop(AF_INET6, &ip6->ip6_src, packet->source_ip, sizeof(packet->source_ip));
        inet_ntop(AF_INET6, &ip6->ip6_dst, packet->dest_ip, sizeof(packet->dest_ip));
        if (ip6->ip6_nxt == IPPROTO_TCP) g_strlcpy(packet->protocol, "TCP", sizeof(packet->protocol));
        else if (ip6->ip6_nxt == IPPROTO_UDP) g_strlcpy(packet->protocol, "UDP", sizeof(packet->protocol));
        else if (ip6->ip6_nxt == IPPROTO_ICMPV6) g_strlcpy(packet->protocol, "ICMP", sizeof(packet->protocol));
        else g_strlcpy(packet->protocol, "OTHER", sizeof(packet->protocol));
        return TRUE;
    }

    g_strlcpy(packet->protocol, "OTHER", sizeof(packet->protocol));
    g_strlcpy(packet->source_ip, "N/A", sizeof(packet->source_ip));
    g_strlcpy(packet->dest_ip, "N/A", sizeof(packet->dest_ip));
    return TRUE;
}

static gpointer packet_capture_worker(gpointer data) {
    PacketCapture *capture = data;
    guint8 buffer[65536];

    while (capture->running) {
        ssize_t n = recvfrom(capture->fd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (n <= 0) {
            if (!capture->running) break;
            continue;
        }

        PacketInfo packet;
        if (parse_packet(buffer, n, &packet) && packet_matches_filter(capture->filter, packet.protocol)) {
            capture->callback(&packet, capture->user_data);
        }
    }
    return NULL;
}

PacketCapture *packet_capture_start(const char *iface, PacketFilter filter, PacketCaptureCallback callback, gpointer user_data, char **error_message) {
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        if (errno == EPERM || errno == EACCES) {
            *error_message = g_strdup("Packet capture requires sudo/root permission");
        } else {
            *error_message = g_strdup_printf("socket(AF_PACKET) failed: %s", strerror(errno));
        }
        return NULL;
    }

    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (iface && *iface) {
        unsigned int ifindex = if_nametoindex(iface);
        if (ifindex == 0) {
            *error_message = g_strdup_printf("Unknown interface: %s", iface);
            close(fd);
            return NULL;
        }
        struct sockaddr_ll addr = {0};
        addr.sll_family = AF_PACKET;
        addr.sll_protocol = htons(ETH_P_ALL);
        addr.sll_ifindex = (int)ifindex;
        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            *error_message = g_strdup_printf("bind interface failed: %s", strerror(errno));
            close(fd);
            return NULL;
        }
    }

    PacketCapture *capture = g_new0(PacketCapture, 1);
    capture->fd = fd;
    capture->running = TRUE;
    capture->filter = filter;
    capture->callback = callback;
    capture->user_data = user_data;
    capture->thread = g_thread_new("packet-capture", packet_capture_worker, capture);
    return capture;
}

void packet_capture_stop(PacketCapture *capture) {
    if (!capture) return;
    capture->running = FALSE;
    shutdown(capture->fd, SHUT_RDWR);
    close(capture->fd);
    if (capture->thread) g_thread_join(capture->thread);
    g_free(capture);
}

static gboolean text_contains_casefold(const char *haystack, const char *needle) {
    if (!needle || !*needle) return TRUE;
    char *h = g_utf8_casefold(haystack ? haystack : "", -1);
    char *n = g_utf8_casefold(needle, -1);
    gboolean found = g_strstr_len(h, -1, n) != NULL;
    g_free(h);
    g_free(n);
    return found;
}

static gboolean connection_matches(ConnectionInfo *info, const char *protocol_filter, const char *search) {
    if (protocol_filter && *protocol_filter && g_strcmp0(protocol_filter, "ALL") != 0 &&
        g_ascii_strcasecmp(info->protocol, protocol_filter) != 0) {
        return FALSE;
    }

    if (!search || !*search) return TRUE;
    char *combined = g_strdup_printf("%s %s %s %s %s",
        info->protocol, info->local_address, info->remote_address, info->state, info->program);
    gboolean ok = text_contains_casefold(combined, search);
    g_free(combined);
    return ok;
}

static ConnectionInfo *parse_ss_line(const char *line) {
    char **tokens = g_strsplit_set(g_strstrip((char *)line), " \t", 0);
    GPtrArray *parts = g_ptr_array_new();
    for (int i = 0; tokens[i]; i++) {
        if (tokens[i][0]) g_ptr_array_add(parts, tokens[i]);
    }

    if (parts->len < 5) {
        g_ptr_array_free(parts, TRUE);
        g_strfreev(tokens);
        return NULL;
    }

    ConnectionInfo *info = g_new0(ConnectionInfo, 1);
    const char *proto = g_ptr_array_index(parts, 0);
    info->protocol = g_ascii_strup(proto, -1);
    info->state = g_strdup(g_ptr_array_index(parts, 1));

    guint local_idx = 4;
    guint remote_idx = 5;
    if (parts->len <= remote_idx) {
        local_idx = 3;
        remote_idx = 4;
    }

    info->local_address = g_strdup(local_idx < parts->len ? g_ptr_array_index(parts, local_idx) : "N/A");
    info->remote_address = g_strdup(remote_idx < parts->len ? g_ptr_array_index(parts, remote_idx) : "N/A");

    GString *program = g_string_new("");
    for (guint i = remote_idx + 1; i < parts->len; i++) {
        if (program->len) g_string_append_c(program, ' ');
        g_string_append(program, g_ptr_array_index(parts, i));
    }
    info->program = g_string_free(program, FALSE);
    if (!info->program[0]) {
        g_free(info->program);
        info->program = g_strdup("N/A");
    }

    g_ptr_array_free(parts, TRUE);
    g_strfreev(tokens);
    return info;
}

GPtrArray *network_manager_get_connections(const char *protocol_filter, const char *search, char **error_message) {
    char *out = NULL;
    char *err = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync("ss -tunap", &out, &err, &status, &error);
    GPtrArray *items = g_ptr_array_new_with_free_func((GDestroyNotify)connection_info_free);

    if (!ok) {
        *error_message = g_strdup(error->message);
        g_error_free(error);
        g_free(out);
        g_free(err);
        return items;
    }
    if (status != 0 && err && *err) {
        *error_message = g_strdup(err);
    }

    char **lines = g_strsplit(out ? out : "", "\n", -1);
    for (int i = 1; lines[i]; i++) {
        if (!lines[i][0]) continue;
        ConnectionInfo *info = parse_ss_line(lines[i]);
        if (!info) continue;
        if (connection_matches(info, protocol_filter, search)) {
            g_ptr_array_add(items, info);
        } else {
            connection_info_free(info);
        }
    }

    g_strfreev(lines);
    g_free(out);
    g_free(err);
    return items;
}

void connection_info_free(ConnectionInfo *info) {
    if (!info) return;
    g_free(info->protocol);
    g_free(info->local_address);
    g_free(info->remote_address);
    g_free(info->state);
    g_free(info->program);
    g_free(info);
}
