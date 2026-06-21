#ifndef FILE_SYSCALL_H
#define FILE_SYSCALL_H

char *read_file_syscall(const char *path);
int write_file_syscall(const char *path, const char *content, char **message);

#endif
