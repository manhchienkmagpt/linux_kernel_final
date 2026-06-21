#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include <glib.h>
#include <sys/types.h>

char *get_process_list(void);
int kill_process_by_pid(pid_t pid, char **error_message);
char *fork_demo(void);

#endif
