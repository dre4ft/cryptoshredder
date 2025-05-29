#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#define KEY_SIZE 32  // AES-256
#define IV_SIZE 16   // AES block size

int remove_after = 0;

void secure_zero(void *v, size_t n) {
    volatile unsigned char *p = v;
    while (n--) *p++ = 0;
}

void shred_file(const char *filepath) {
    FILE *in = fopen(filepath, "rb+");
    if (!in) {
        perror("fopen");
        return;
    }

    fseek(in, 0, SEEK_END);
    long filesize = ftell(in);
    if (filesize <= 0) {
        fclose(in);
        return;
    }
    fseek(in, 0, SEEK_SET);

    unsigned char *plaintext = malloc(filesize);
    if (!plaintext) {
        perror("malloc");
        fclose(in);
        return;
    }

    if (fread(plaintext, 1, filesize, in) != filesize) {
        perror("fread");
        free(plaintext);
        fclose(in);
        return;
    }

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];

    if (!RAND_bytes(key, sizeof(key)) || !RAND_bytes(iv, sizeof(iv))) {
        fprintf(stderr, "RAND_bytes failed\n");
        free(plaintext);
        fclose(in);
        return;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        perror("EVP_CIPHER_CTX_new");
        free(plaintext);
        fclose(in);
        return;
    }

    unsigned char *ciphertext = malloc(filesize + EVP_MAX_BLOCK_LENGTH);
    int len, ciphertext_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, filesize);
    ciphertext_len += len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    rewind(in);
    fwrite(ciphertext, 1, ciphertext_len, in);
    fflush(in);
    ftruncate(fileno(in), ciphertext_len);
    fclose(in);

    secure_zero(key, sizeof(key));
    secure_zero(iv, sizeof(iv));
    secure_zero(plaintext, filesize);

    free(plaintext);
    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);

    printf("[OK] Encrypted: %s\n", filepath);

    if (remove_after) {
        if (remove(filepath) == 0) {
            printf("[DEL] Deleted: %s\n", filepath);
        } else {
            perror("[ERR] Failed to delete");
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
            perror("[ERR] Failed to remove directory");
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
