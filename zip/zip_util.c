#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

#define MAX_PATH 4096
#define MAX_CMD 8192

/* Function prototypes */
int zip_files(const char *output_file, char **files, int file_count);
int unzip_archive(const char *archive_path, const char *dest_dir);
int rezip_archive(const char *archive_path, char **files, int file_count);
void print_usage(const char *prog_name);
int check_command_available(const char *cmd);

/* Print usage information */
void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  %s [OPTIONS] <files/folders...>\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <output.zip>     Specify output zip file (default: archive.zip)\n");
    printf("  -u <archive.zip>    Unzip the specified archive\n");
    printf("  -r <archive.zip> <files...>  Rezip: unzip archive, add files, then rezip\n");
    printf("  -h                  Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s file1.txt file2.txt           # Create archive.zip with files\n", prog_name);
    printf("  %s -o myzip.zip folder/          # Zip folder into myzip.zip\n", prog_name);
    printf("  %s -u archive.zip                # Extract archive.zip\n", prog_name);
    printf("  %s -r archive.zip newfile.txt    # Add newfile.txt to archive.zip\n", prog_name);
}

/* Check if a command is available */
int check_command_available(const char *cmd) {
    char check_cmd[256];
    snprintf(check_cmd, sizeof(check_cmd), "which %s > /dev/null 2>&1", cmd);
    return system(check_cmd) == 0;
}

/* Check if a path is a directory */
int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/* Escape single quotes in a string for shell commands */
char* escape_single_quotes(const char *str) {
    size_t len = strlen(str);
    size_t new_len = len;

    /* Count single quotes */
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\'') {
            new_len += 3; /* Replace ' with '\'' */
        }
    }

    char *escaped = malloc(new_len + 1);
    if (!escaped) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\'') {
            escaped[j++] = '\'';
            escaped[j++] = '\\';
            escaped[j++] = '\'';
            escaped[j++] = '\'';
        } else {
            escaped[j++] = str[i];
        }
    }
    escaped[j] = '\0';

    return escaped;
}

/* Create a zip archive with the specified files */
int zip_files(const char *output_file, char **files, int file_count) {
    if (!check_command_available("zip")) {
        fprintf(stderr, "Error: 'zip' command not found. Please install zip utility.\n");
        return -1;
    }

    printf("Creating archive: %s\n", output_file);

    /* Build zip command */
    char cmd[MAX_CMD];
    int cmd_len = snprintf(cmd, MAX_CMD, "zip -r ");

    /* Add output file */
    char *escaped_output = escape_single_quotes(output_file);
    if (!escaped_output) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }
    cmd_len += snprintf(cmd + cmd_len, MAX_CMD - cmd_len, "'%s' ", escaped_output);
    free(escaped_output);

    /* Add all input files */
    for (int i = 0; i < file_count; i++) {
        if (cmd_len >= MAX_CMD - 100) {
            fprintf(stderr, "Error: Command too long\n");
            return -1;
        }

        char *escaped_file = escape_single_quotes(files[i]);
        if (!escaped_file) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return -1;
        }

        cmd_len += snprintf(cmd + cmd_len, MAX_CMD - cmd_len, "'%s' ", escaped_file);
        free(escaped_file);
    }

    /* Execute zip command */
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Error: Zip operation failed\n");
        return -1;
    }

    printf("Archive created successfully: %s\n", output_file);
    return 0;
}

/* Extract a zip archive */
int unzip_archive(const char *archive_path, const char *dest_dir) {
    if (!check_command_available("unzip")) {
        fprintf(stderr, "Error: 'unzip' command not found. Please install unzip utility.\n");
        return -1;
    }

    printf("Extracting archive: %s\n", archive_path);

    /* Build unzip command */
    char cmd[MAX_CMD];

    char *escaped_archive = escape_single_quotes(archive_path);
    if (!escaped_archive) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    char *escaped_dest = escape_single_quotes(dest_dir);
    if (!escaped_dest) {
        free(escaped_archive);
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    snprintf(cmd, MAX_CMD, "unzip -o '%s' -d '%s'", escaped_archive, escaped_dest);
    free(escaped_archive);
    free(escaped_dest);

    /* Execute unzip command */
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Error: Unzip operation failed\n");
        return -1;
    }

    printf("Extraction complete!\n");
    return 0;
}

/* Rezip: unzip archive, add files, then rezip */
int rezip_archive(const char *archive_path, char **files, int file_count) {
    if (!check_command_available("zip") || !check_command_available("unzip")) {
        fprintf(stderr, "Error: 'zip' or 'unzip' command not found.\n");
        return -1;
    }

    /* Convert archive_path to absolute path */
    char absolute_archive_path[MAX_PATH];
    if (archive_path[0] != '/') {
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            fprintf(stderr, "Error: Cannot get current directory\n");
            return -1;
        }
        snprintf(absolute_archive_path, MAX_PATH, "%s/%s", cwd, archive_path);
    } else {
        snprintf(absolute_archive_path, MAX_PATH, "%s", archive_path);
    }

    char temp_dir[] = "/tmp/rezip_XXXXXX";

    /* Create temporary directory */
    if (mkdtemp(temp_dir) == NULL) {
        fprintf(stderr, "Error: Cannot create temporary directory\n");
        return -1;
    }

    printf("Rezipping archive: %s\n", archive_path);

    /* Extract existing archive to temp directory */
    if (unzip_archive(archive_path, temp_dir) != 0) {
        fprintf(stderr, "Error: Failed to extract archive\n");
        char cmd[MAX_PATH + 20];
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", temp_dir);
        system(cmd);
        return -1;
    }

    /* Copy new files to temp directory */
    for (int i = 0; i < file_count; i++) {
        char *file = files[i];
        char cmd[MAX_CMD];

        char *escaped_file = escape_single_quotes(file);
        char *escaped_temp = escape_single_quotes(temp_dir);

        if (!escaped_file || !escaped_temp) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            if (escaped_file) free(escaped_file);
            if (escaped_temp) free(escaped_temp);
            continue;
        }

        snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s/'", escaped_file, escaped_temp);
        free(escaped_file);
        free(escaped_temp);

        int result = system(cmd);
        if (result == 0) {
            printf("Added to rezip: %s\n", file);
        } else {
            fprintf(stderr, "Warning: Failed to copy '%s'\n", file);
        }
    }

    /* Create backup of original archive */
    char backup_path[MAX_PATH];
    snprintf(backup_path, MAX_PATH, "%s.backup", absolute_archive_path);
    rename(absolute_archive_path, backup_path);

    /* Change to temp directory and create new archive */
    char saved_cwd[MAX_PATH];
    if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL) {
        fprintf(stderr, "Error: Cannot get current directory\n");
        rename(backup_path, absolute_archive_path);
        return -1;
    }

    if (chdir(temp_dir) != 0) {
        fprintf(stderr, "Error: Cannot change to temporary directory\n");
        rename(backup_path, absolute_archive_path);
        return -1;
    }

    /* Build zip command to create archive from temp directory contents */
    char cmd[MAX_CMD];
    char *escaped_archive = escape_single_quotes(absolute_archive_path);
    if (!escaped_archive) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (chdir(saved_cwd) != 0) {
            fprintf(stderr, "Warning: Failed to return to original directory\n");
        }
        rename(backup_path, absolute_archive_path);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "zip -r '%s' .", escaped_archive);
    free(escaped_archive);

    int result = system(cmd);

    /* Return to original directory */
    if (chdir(saved_cwd) != 0) {
        fprintf(stderr, "Warning: Failed to return to original directory\n");
    }

    if (result != 0) {
        fprintf(stderr, "Error: Failed to create new archive\n");
        rename(backup_path, absolute_archive_path);
        return -1;
    }

    /* Clean up */
    char cleanup_cmd[MAX_PATH + 20];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf '%s'", temp_dir);
    if (system(cleanup_cmd) != 0) {
        fprintf(stderr, "Warning: Failed to clean up temporary directory\n");
    }
    remove(backup_path);

    printf("Rezip complete: %s\n", archive_path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    /* Parse command line arguments */
    int opt;
    char *output_file = "archive.zip";
    int unzip_mode = 0;
    int rezip_mode = 0;
    char *archive_path = NULL;

    while ((opt = getopt(argc, argv, "o:u:r:h")) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                break;
            case 'u':
                unzip_mode = 1;
                archive_path = optarg;
                break;
            case 'r':
                rezip_mode = 1;
                archive_path = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Execute requested operation */
    if (unzip_mode) {
        /* Unzip mode */
        const char *dest_dir = ".";
        return unzip_archive(archive_path, dest_dir);
    } else if (rezip_mode) {
        /* Rezip mode */
        if (optind >= argc) {
            fprintf(stderr, "Error: No files specified for rezip operation\n");
            return 1;
        }
        return rezip_archive(archive_path, &argv[optind], argc - optind);
    } else {
        /* Zip mode */
        if (optind >= argc) {
            fprintf(stderr, "Error: No files specified to zip\n");
            print_usage(argv[0]);
            return 1;
        }
        return zip_files(output_file, &argv[optind], argc - optind);
    }

    return 0;
}
