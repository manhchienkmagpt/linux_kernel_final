#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <glib.h>

typedef enum {
    PACKET_FILTER_ALL,
    PACKET_FILTER_TCP,
    PACKET_FILTER_UDP,
    PACKET_FILTER_ICMP
} PacketFilter;

typedef struct {
    char time_text[32];
    char protocol[16];
    char source_ip[64];
    char dest_ip[64];
    guint length;
} PacketInfo;

typedef struct {
    char *protocol;
    char *local_address;
    char *remote_address;
    char *state;
    char *program;
} ConnectionInfo;

typedef struct {
    guint64 rx_bytes;
    guint64 tx_bytes;
} TrafficStats;

typedef void (*PacketCaptureCallback)(const PacketInfo *packet, gpointer user_data);

typedef struct PacketCapture PacketCapture;

GPtrArray *network_manager_list_interfaces(void);
gboolean network_manager_read_traffic(const char *iface, TrafficStats *stats, char **error_message);
char *network_manager_format_bytes(guint64 bytes);
char *network_manager_format_speed(double bytes_per_second);

PacketFilter packet_filter_from_text(const char *text);
PacketCapture *packet_capture_start(const char *iface, PacketFilter filter, PacketCaptureCallback callback, gpointer user_data, char **error_message);
void packet_capture_stop(PacketCapture *capture);

GPtrArray *network_manager_get_connections(const char *protocol_filter, const char *search, char **error_message);
void connection_info_free(ConnectionInfo *info);

#endif
