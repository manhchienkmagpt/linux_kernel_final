#include "file_syscall.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *read_file_syscall(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return g_strdup_printf("open failed: %s", strerror(errno));

    GString *result = g_string_new("");
    char buffer[1024];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        g_string_append_len(result, buffer, n);
    }
    if (n < 0) g_string_append_printf(result, "\nread failed: %s", strerror(errno));
    close(fd);
    return g_string_free(result, FALSE);
}

int write_file_syscall(const char *path, const char *content, char **message) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        *message = g_strdup_printf("open failed: %s", strerror(errno));
        return -1;
    }
    size_t len = strlen(content);
    ssize_t n = write(fd, content, len);
    close(fd);
    if (n < 0 || (size_t)n != len) {
        *message = g_strdup_printf("write failed: %s", strerror(errno));
        return -1;
    }
    *message = g_strdup_printf("Wrote %zd bytes to %s", n, path);
    return 0;
}
