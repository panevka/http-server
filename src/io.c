#include "io.h"
#include "logging.h"
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void close_file(FILE *f) {
  if (f && fclose(f) != 0) {
    perror("Warning: could not close the file");
  }
}

off_t read_file(const char *path, char *file_buffer, size_t len) {
  FILE *fptr = NULL;
  off_t file_size;

  // Open file
  fptr = fopen(path, "rb");
  if (!fptr) {
    fprintf(stderr, "Could not open file %s: %s\n", path, strerror(errno));
    return -1;
  }

  // Determine file size
  if (fseeko(fptr, 0, SEEK_END) != 0) {
    perror("Could not go to file end");
    close_file(fptr);
    return -1;
  }
  file_size = ftello(fptr);
  if (file_size == -1) {
    perror("Could not verify file size");
    close_file(fptr);
    return -1;
  }
  rewind(fptr);

  // Read into file buffer
  fread(file_buffer, 1, len, fptr);
  if (ferror(fptr)) {
    perror("fread failed");
    close_file(fptr);
    return -1;
  }
  close_file(fptr);

  printf("%s\n", file_buffer);

  return file_size;
}

/**
 * Writes the contents of a directory into an HTML file.
 *
 * Scans `directory_path` and saves entries into `file_save_path`
 * as an HTML list (<li>...</li>). Regular files are written as
 * "name", directories as "name/".
 *
 * @param directory_path Path to the directory to scan.
 * @param file_save_path Path to the HTML file to write.
 * @return 0 on success, -1 on failure.
 */
int write_dir_entries_html(char *directory_path, const char *file_save_path) {

  DIR *scanned_directory = NULL;
  FILE *output_file = NULL;
  struct dirent *dir_entry;
  struct stat entry_info;

  // Open directory that will have their contents searched
  scanned_directory = opendir(directory_path);
  if (scanned_directory == NULL) {
    log_msg(MSG_ERROR, true, "Could not open directory %s.", directory_path);
    goto FAIL;
  }

  // Open file where the content list will be saved
  output_file = fopen(file_save_path, "w");
  if (output_file == NULL) {
    log_msg(MSG_ERROR, true, "Could not open file %s.", file_save_path);
    goto FAIL;
  }

  // Loop for fetching directory entries
  while (true) {

    // Fetch directory entry
    errno = 0;
    dir_entry = readdir(scanned_directory);
    if (errno != 0) {
      log_msg(MSG_ERROR, true, "Fetching directory entry failed.");
      goto FAIL;
    }
    if (dir_entry == NULL) {
      goto SUCCESS;
    }

    // Concatenate directory path with entry name
    char entry_path[MAX_FILE_PATH_LENGTH + 1];
    int written = snprintf(entry_path, sizeof(entry_path), "%s/%s",
                           directory_path, dir_entry->d_name);
    if (written < 0) {
      log_msg(MSG_ERROR, true, "Could not write to entry path buffer.");
      goto FAIL;
    }
    if ((size_t)written >= sizeof(entry_path)) {
      log_msg(MSG_ERROR, true,
              "Could not write all data to entry path buffer.");
      goto FAIL;
    }

    // Get info about directory entry
    if (stat(entry_path, &entry_info) != 0) {
      log_msg(MSG_ERROR, true,
              "Could not obtain information about directory entry: %s.",
              entry_path);
      goto FAIL;
    }

    // Write entry to an HTML file
    if (S_ISREG(entry_info.st_mode)) {
      fprintf(output_file, "<li>%s</li><br>", dir_entry->d_name);
    } else if (S_ISDIR(entry_info.st_mode)) {
      fprintf(output_file, "<li>%s/</li><br>", dir_entry->d_name);
    }
  }

SUCCESS:
  fclose(output_file);
  closedir(scanned_directory);
  return 0;

FAIL:
  if (output_file) {
    fclose(output_file);
  }
  if (scanned_directory) {
    closedir(scanned_directory);
  }
  return -1;
}
