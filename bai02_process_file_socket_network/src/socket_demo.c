#include "socket_demo.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int server_fd = -1;
static gboolean server_running = FALSE;
static pthread_t server_thread;
static SocketLogFn server_log = NULL;
static void *server_log_data = NULL;

static void log_msg(const char *msg) {
    if (server_log) server_log(msg, server_log_data);
}

static void *server_loop(void *arg) {
    int port = GPOINTER_TO_INT(arg);
    char line[128];
    snprintf(line, sizeof(line), "Server listening on port %d", port);
    log_msg(line);

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client < 0) {
            if (server_running) log_msg("accept failed.");
            continue;
        }
        char ip[64];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        snprintf(line, sizeof(line), "Client connected: %s:%d", ip, ntohs(client_addr.sin_port));
        log_msg(line);

        char buffer[512] = {0};
        ssize_t n = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            snprintf(line, sizeof(line), "Received: %.100s", buffer);
            log_msg(line);
            char reply[640];
            snprintf(reply, sizeof(reply), "ACK from server. Received: %.500s", buffer);
            send(client, reply, strlen(reply), 0);
        }
        close(client);
    }
    return NULL;
}

int socket_server_start(int port, SocketLogFn log_fn, void *user_data, char **message) {
    if (server_running) {
        *message = g_strdup("Server is already running.");
        return -1;
    }
    server_log = log_fn;
    server_log_data = user_data;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        *message = g_strdup_printf("socket failed: %s", strerror(errno));
        return -1;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(server_fd, 5) < 0) {
        *message = g_strdup_printf("bind/listen failed: %s", strerror(errno));
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    server_running = TRUE;
    if (pthread_create(&server_thread, NULL, server_loop, GINT_TO_POINTER(port)) != 0) {
        *message = g_strdup("pthread_create failed.");
        server_running = FALSE;
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    *message = g_strdup("Server started.");
    return 0;
}

void socket_server_stop(SocketLogFn log_fn, void *user_data) {
    (void)log_fn;
    (void)user_data;
    if (!server_running) return;
    server_running = FALSE;
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    server_fd = -1;
    pthread_join(server_thread, NULL);
    log_msg("Server stopped.");
}

char *socket_client_send(const char *host, int port, const char *message) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return g_strdup_printf("socket failed: %s", strerror(errno));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(fd);
        return g_strdup("Invalid IPv4 address.");
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        char *msg = g_strdup_printf("connect failed: %s", strerror(errno));
        close(fd);
        return msg;
    }
    if (send(fd, message, strlen(message), 0) < 0) {
        char *msg = g_strdup_printf("send failed: %s", strerror(errno));
        close(fd);
        return msg;
    }
    char buffer[512] = {0};
    ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    close(fd);
    if (n < 0) return g_strdup_printf("recv failed: %s", strerror(errno));
    return g_strdup_printf("Sent: %s\nServer response: %s", message, buffer);
}
