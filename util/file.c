// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#define _FILE_OFFSET_BITS 64
#define _DEFAULT_SOURCE 1

#include "util/util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

void input_file_destroy(struct input_file *file) {
    int r = munmap(file->data, file->size);
    if (r != 0) {
        LOG_ERROR_ERRNO(errno, "munmap");
        abort();
    }
}

// Benign comparison between off_t and size_t below.
#pragma GCC diagnostic ignored "-Wsign-compare"

int input_file_read(struct input_file *file, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        LOG_ERROR_ERRNO(errno, "open %s", filename);
        return -1;
    }
    struct stat st;
    int r;
    r = fstat(fd, &st);
    if (r == -1) {
        LOG_ERROR_ERRNO(errno, "stat %s", filename);
        goto error;
    }
    off_t size = st.st_size;
    if (size > (size_t)-1) {
        LOG_ERROR("read %s: file too large", filename);
        goto error;
    }
    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOG_ERROR_ERRNO(errno, "mmap %s", filename);
        goto error;
    }
    *file = (struct input_file){
        .data = ptr,
        .size = size,
    };
    close(fd);
    return 0;
error:
    close(fd);
    return -1;
}

#define MAX_IOVEC 8

int output_file_write(const char *filename, const struct bytestring *data,
                      size_t count) {
    if (count > MAX_IOVEC) {
        LOG_ERROR("too many output file segments");
        abort();
    }
    struct iovec iov[MAX_IOVEC], *ioq = iov;
    for (size_t i = 0; i < count; i++) {
        if (data[i].size > 0) {
            *ioq++ = (struct iovec){
                .iov_base = (void *)data[i].ptr,
                .iov_len = data[i].size,
            };
        }
    }
    int errcode;
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        errcode = errno;
        goto error;
    }
    struct iovec *iop = iov;
    while (iop < ioq) {
        ssize_t amt = writev(fd, iop, ioq - iop);
        if (amt < 0) {
            errcode = errno;
            if (errcode != EINTR) {
                goto error;
            }
        } else {
            size_t rem = amt;
            while (rem > 0 && rem >= iop->iov_len) {
                rem -= iop->iov_len;
                iop++;
            }
            if (rem > 0) {
                *iop = (struct iovec){
                    .iov_base = (char *)iop->iov_base + rem,
                    .iov_len = iop->iov_len - rem,
                };
            }
        }
    }
    int r = close(fd);
    if (r == -1) {
        errcode = errno;
        goto error;
    }
    return 0;

error:
    LOG_ERROR_ERRNO(errcode, "write %s", filename);
    return -1;
}
