#ifndef IO_H
#define IO_H
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_FILE_PATH_LENGTH 4096
#define MAX_FILE_SIZE 1048576

off_t read_file(const char *path, char *file_buffer, size_t len);

int write_dir_entries_html(char *directory_path, const char *file_save_path);

int get_file_fd(char *path);

char *get_mime_type(char *path);

#endif
