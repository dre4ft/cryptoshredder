#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#define KEY_SIZE 4096

void secure_zero(void *v, size_t n) {
    volatile unsigned char *p = v;
    while (n--) *p++ = 0;
}

int remove_after = 0; // Global

void shred_file(const char *filepath) {
    int fd = open(filepath, O_RDWR);
    if (fd < 0) {
        perror("open");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return;
    }

    if (!S_ISREG(st.st_mode) || st.st_size == 0) {
        close(fd);
        return;
    }

    void *map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    unsigned char *key = malloc(KEY_SIZE);
    if (!key) {
        perror("malloc");
        munmap(map, st.st_size);
        close(fd);
        return;
    }

    int randfd = open("/dev/urandom", O_RDONLY);
    if (randfd < 0) {
        perror("open /dev/urandom");
        free(key);
        munmap(map, st.st_size);
        close(fd);
        return;
    }

    if (read(randfd, key, KEY_SIZE) != KEY_SIZE) {
        perror("read /dev/urandom");
        close(randfd);
        free(key);
        munmap(map, st.st_size);
        close(fd);
        return;
    }
    close(randfd);

    unsigned char *data = (unsigned char *)map;
    for (off_t i = 0; i < st.st_size; i++) {
        data[i] ^= key[i % KEY_SIZE];
    }

    if (msync(map, st.st_size, MS_SYNC) < 0) {
        perror("msync");
    }

    secure_zero(key, KEY_SIZE);
    free(key);
    munmap(map, st.st_size);
    close(fd);

    printf("[OK] Shredded: %s\n", filepath);

    if (remove_after) {
        if (remove(filepath) == 0) {
            printf("[DEL] Deleted: %s\n", filepath);
        } else {
            perror("[ERR] File deletion failed");
        }
    }
}

void shred_directory(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) < 0) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            shred_directory(path);
        } else if (S_ISREG(st.st_mode)) {
            shred_file(path);
        }
    }

    closedir(dir);

    if (remove_after) {
        if (rmdir(dirpath) == 0) {
            printf("[DEL] Directory deleted: %s\n", dirpath);
        } else {
            perror("[ERR] Directory deletion failed");
        }
    }
}

int main(int argc, char *argv[]) {
    char *file = NULL;
    char *dir = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            dir = argv[++i];
        } else if (strcmp(argv[i], "-frm") == 0 && i + 1 < argc) {
            file = argv[++i];
            remove_after = 1;
        } else if (strcmp(argv[i], "-drm") == 0 && i + 1 < argc) {
            dir = argv[++i];
            remove_after = 1;
        } else {
            fprintf(stderr, "Usage: %s -f <file> | -d <directory> [-frm|-drm]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (file) {
        shred_file(file);
    } else if (dir) {
        shred_directory(dir);
    } else {
        fprintf(stderr, "Error: specify -f <file> or -d <directory>\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
