#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_SAF_FILES 256
#define MAX_PATH_LEN 512

typedef struct {
    char path[MAX_PATH_LEN];
    int fd;
} SafFileEntry;

static SafFileEntry sSafFiles[MAX_SAF_FILES];
static int sSafFileCount = 0;
static int sSafRomFd = -1;

int saf_get_rom_fd(void) {
    return sSafRomFd;
}

int saf_find_fd(const char *path) {
    if (!path) return -1;
    for (int i = 0; i < sSafFileCount; i++) {
        if (strstr(path, sSafFiles[i].path) != NULL || strcmp(path, sSafFiles[i].path) == 0) {
            // Return a duplicated file descriptor so multiple reads/fcloses do not invalidate the original
            return dup(sSafFiles[i].fd);
        }
    }
    return -1;
}

FILE *saf_fopen(const char *path, const char *mode) {
    int fd = saf_find_fd(path);
    if (fd >= 0) {
        return fdopen(fd, mode);
    }
    return fopen(path, mode);
}

JNIEXPORT void JNICALL
Java_com_maniscat2_sm64coopdx_sm64coopdxActivity_nativeSetSafRomFd(JNIEnv *env, jclass clazz, jint fd) {
    sSafRomFd = fd;
}

JNIEXPORT void JNICALL
Java_com_maniscat2_sm64coopdx_sm64coopdxActivity_nativeSetSafAssetFd(JNIEnv *env, jclass clazz, jstring relativePath, jint fd) {
    if (sSafFileCount >= MAX_SAF_FILES) return;
    const char *nativePath = (*env)->GetStringUTFChars(env, relativePath, 0);
    if (nativePath) {
        strncpy(sSafFiles[sSafFileCount].path, nativePath, MAX_PATH_LEN - 1);
        sSafFiles[sSafFileCount].path[MAX_PATH_LEN - 1] = '\0';
        sSafFiles[sSafFileCount].fd = fd;
        sSafFileCount++;
        (*env)->ReleaseStringUTFChars(env, relativePath, nativePath);
    }
}