#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define MAX_PATH_LEN 1024
#define SLEEP_INTERVAL_MICROSECONDS 100000

const char *dirr_a;
const char *dirr_b;

void synchronize_directories(const char *dir_a, const char *dir_b) {
    DIR *dir_stream_a = opendir(dir_a);
    if (dir_stream_a == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry_a;
    while ((entry_a = readdir(dir_stream_a)) != NULL) {
        if (strcmp(entry_a->d_name, ".") == 0 || strcmp(entry_a->d_name, "..") == 0)
            continue;

        char path_a[MAX_PATH_LEN];
        snprintf(path_a, MAX_PATH_LEN, "%s/%s", dir_a, entry_a->d_name);

        char path_b[MAX_PATH_LEN];
        snprintf(path_b, MAX_PATH_LEN, "%s/%s", dir_b, entry_a->d_name);

        struct stat stat_a;
        if (lstat(path_a, &stat_a) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(stat_a.st_mode)) {
            synchronize_directories(path_a, path_b);
        } else {
            struct stat stat_b;
            if (lstat(path_b, &stat_b) == -1) {
                if (link(path_a, path_b) == -1) {
                    perror("link");
                }
            }
        }
    }
    closedir(dir_stream_a);

    DIR *dir_stream_b = opendir(dir_b);
    if (dir_stream_b == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry_b;
    while ((entry_b = readdir(dir_stream_b)) != NULL) {
        if (strcmp(entry_b->d_name, ".") == 0 || strcmp(entry_b->d_name, "..") == 0)
            continue;

        char path_a[MAX_PATH_LEN];
        snprintf(path_a, MAX_PATH_LEN, "%s/%s", dir_a, entry_b->d_name);

        char path_b[MAX_PATH_LEN];
        snprintf(path_b, MAX_PATH_LEN, "%s/%s", dir_b, entry_b->d_name);

        struct stat stat_b;
        if (lstat(path_b, &stat_b) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(stat_b.st_mode)) {
            struct stat stat_a;
            if (lstat(path_a, &stat_a) == -1) {
                if (unlink(path_b) == -1) {
                    perror("unlink");
                }
            }
        } else {
            struct stat stat_a;
            if (lstat(path_a, &stat_a) == -1) {
                if (unlink(path_b) == -1) {
                    perror("unlink");
                }
            }
        }
    }
    closedir(dir_stream_b);
}

void *monitor_and_sync(void *arg) {
    while (1) {
        synchronize_directories(dirr_a, dirr_b);
        usleep(SLEEP_INTERVAL_MICROSECONDS);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory_a> <directory_b>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    dirr_a = argv[1];
    dirr_b = argv[2];

    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_and_sync, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    synchronize_directories(dirr_a, dirr_b);

    if (pthread_join(monitor_thread, NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    return 0;
}
