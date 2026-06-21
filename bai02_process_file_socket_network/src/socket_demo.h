#ifndef SOCKET_DEMO_H
#define SOCKET_DEMO_H

#include <glib.h>

typedef void (*SocketLogFn)(const char *message, void *user_data);

int socket_server_start(int port, SocketLogFn log_fn, void *user_data, char **message);
void socket_server_stop(SocketLogFn log_fn, void *user_data);
char *socket_client_send(const char *host, int port, const char *message);

#endif
