#ifndef READTIMEOUT_H
#define READTIMEOUT_H

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

ssize_t readTimeOut(int fd, void *buf, size_t count, u_int16_t t);
ssize_t completeReadTimeout(int fd, void *dst, size_t n, u_int16_t t);

ssize_t completeSend(int fd, int fd_in, size_t n);
void recvFile(int fd, const char *filename, u_int16_t t);
ssize_t sendCustomFile(int fd, const char *filename);

#endif // READTIMEOUT_H
