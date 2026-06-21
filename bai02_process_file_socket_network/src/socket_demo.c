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
static GMutex server_clients_mutex;
static GArray *server_clients = NULL;

static int client_fd = -1;
static gboolean client_running = FALSE;
static pthread_t client_thread;
static SocketLogFn client_log = NULL;
static void *client_log_data = NULL;

static void log_msg(const char *msg) {
    if (server_log) server_log(msg, server_log_data);
}

static void client_log_msg(const char *msg) {
    if (client_log) client_log(msg, client_log_data);
}

static void remove_server_client(int fd) {
    g_mutex_lock(&server_clients_mutex);
    if (server_clients) {
        for (guint i = 0; i < server_clients->len; i++) {
            int current = g_array_index(server_clients, int, i);
            if (current == fd) {
                g_array_remove_index(server_clients, i);
                break;
            }
        }
    }
    g_mutex_unlock(&server_clients_mutex);
}

static void broadcast_line(const char *line) {
    char *packet = g_strdup_printf("%s\n", line);
    g_mutex_lock(&server_clients_mutex);
    if (server_clients) {
        for (guint i = 0; i < server_clients->len; i++) {
            int fd = g_array_index(server_clients, int, i);
            if (send(fd, packet, strlen(packet), 0) < 0) {
                shutdown(fd, SHUT_RDWR);
                g_array_remove_index(server_clients, i);
                i--;
            }
        }
    }
    g_mutex_unlock(&server_clients_mutex);
    g_free(packet);
}

static void *client_handler_loop(void *arg) {
    int client = GPOINTER_TO_INT(arg);
    char buffer[512];

    while (server_running) {
        ssize_t n = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        g_strchomp(buffer);
        if (!buffer[0]) continue;

        char line[640];
        snprintf(line, sizeof(line), "Client: %.600s", buffer);
        log_msg(line);
        broadcast_line(line);
    }

    remove_server_client(client);
    close(client);
    log_msg("Client disconnected.");
    return NULL;
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

        g_mutex_lock(&server_clients_mutex);
        if (!server_clients) server_clients = g_array_new(FALSE, FALSE, sizeof(int));
        g_array_append_val(server_clients, client);
        g_mutex_unlock(&server_clients_mutex);

        pthread_t handler;
        if (pthread_create(&handler, NULL, client_handler_loop, GINT_TO_POINTER(client)) == 0) {
            pthread_detach(handler);
        } else {
            remove_server_client(client);
            close(client);
            log_msg("pthread_create for client failed.");
        }
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
    g_mutex_lock(&server_clients_mutex);
    if (!server_clients) server_clients = g_array_new(FALSE, FALSE, sizeof(int));
    g_mutex_unlock(&server_clients_mutex);
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

    g_mutex_lock(&server_clients_mutex);
    if (server_clients) {
        for (guint i = 0; i < server_clients->len; i++) {
            int fd = g_array_index(server_clients, int, i);
            shutdown(fd, SHUT_RDWR);
        }
    }
    g_mutex_unlock(&server_clients_mutex);
    log_msg("Server stopped.");
}

int socket_server_broadcast_message(const char *message, char **status_message) {
    if (!server_running) {
        *status_message = g_strdup("Server is not running.");
        return -1;
    }
    if (!message || !*message) {
        *status_message = g_strdup("Message is required.");
        return -1;
    }

    char line[640];
    snprintf(line, sizeof(line), "Server: %.600s", message);
    log_msg(line);
    broadcast_line(line);
    *status_message = g_strdup(line);
    return 0;
}

static void *client_receive_loop(void *arg) {
    (void)arg;
    char buffer[640];

    while (client_running) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            if (client_running) client_log_msg("Disconnected from server.");
            break;
        }
        buffer[n] = '\0';
        g_strchomp(buffer);
        client_log_msg(buffer);
    }

    client_running = FALSE;
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    return NULL;
}

int socket_client_connect(const char *host, int port, SocketLogFn log_fn, void *user_data, char **message) {
    if (client_running) {
        *message = g_strdup("Client is already connected.");
        return -1;
    }

    client_log = log_fn;
    client_log_data = user_data;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        *message = g_strdup_printf("socket failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(client_fd);
        client_fd = -1;
        *message = g_strdup("Invalid IPv4 address.");
        return -1;
    }
    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        *message = g_strdup_printf("connect failed: %s", strerror(errno));
        close(client_fd);
        client_fd = -1;
        return -1;
    }

    client_running = TRUE;
    if (pthread_create(&client_thread, NULL, client_receive_loop, NULL) != 0) {
        *message = g_strdup("pthread_create failed.");
        client_running = FALSE;
        close(client_fd);
        client_fd = -1;
        return -1;
    }
    pthread_detach(client_thread);

    *message = g_strdup("Connected to server.");
    return 0;
}

void socket_client_disconnect(void) {
    if (!client_running && client_fd < 0) return;
    client_running = FALSE;
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    client_fd = -1;
}

int socket_client_send_message(const char *message, char **status_message) {
    if (!client_running || client_fd < 0) {
        *status_message = g_strdup("Client is not connected. Click Start in Client mode first.");
        return -1;
    }
    if (!message || !*message) {
        *status_message = g_strdup("Message is required.");
        return -1;
    }
    if (send(client_fd, message, strlen(message), 0) < 0) {
        *status_message = g_strdup_printf("send failed: %s", strerror(errno));
        return -1;
    }
    *status_message = g_strdup("Message sent.");
    return 0;
}
