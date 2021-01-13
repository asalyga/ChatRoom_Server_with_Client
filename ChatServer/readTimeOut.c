#include "readtimeout.h"

// read is a blocking statement and can block a program forever,
// hence it is recommended to use a timeout system to make it a blocking for a specified time

ssize_t readTimeOut(int fd, void *buf, size_t count, u_int16_t t)
{
    assert(fd > 0);

    struct timeval t_sec;
    // see "man select" or "man 2 select_tut" for details
    fd_set readfds, writefds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    t_sec.tv_sec = t;
    t_sec.tv_usec = 0;
    FD_SET(fd, &readfds);

    // checks if there's something available to read within in timeout value
    if(select(fd+1, &readfds, &writefds, &exceptfds, &t_sec) < 0) {
        perror("select");
        return -1;
    }
    else if (FD_ISSET(fd, &readfds))
        return read(fd, buf, count);
    else
        return -1;
}


// one read command doesnt necessarily read complete number of bytes,
// therefore a completeRead function is required to guarantee that it reads as much bytes as expected
ssize_t completeReadTimeout(int fd, void *dst, size_t n, u_int16_t t)
{
    assert(dst != NULL);
    assert(fd > 0);

    ssize_t bytes_recv = 0;
    size_t bytes_left = n;
    u_int8_t *dst_buf = (u_int8_t *) (dst);

    while(bytes_left) {
        bytes_recv = readTimeOut(fd, dst_buf, bytes_left, t);
        if(bytes_recv <= 0) {
            perror("read");
            return -1;
        }
        assert((ssize_t)(bytes_left) >= bytes_recv);
        bytes_left -= (ssize_t)(bytes_recv);
        dst_buf += bytes_recv;
    }
    return (ssize_t)(n);
}

ssize_t completeSend(int fd, int fd_in, size_t n)
{
    ssize_t bytes_sent = 0;
    size_t bytes_left = n;
    long int offset = 0;

    while(bytes_left > 0) {
        bytes_sent = sendfile(fd, fd_in, &offset, bytes_left);
        if(bytes_sent <= 0) {
            perror("sendfile");
            return -1;
        }
        assert((ssize_t)(bytes_left) >= bytes_sent);
        bytes_left -= (ssize_t)(bytes_sent);
        offset += bytes_sent;
    }
    return (ssize_t)(n);
}


void recvFile(int fd, const char *filename, u_int16_t t)
{
    assert(filename != NULL);
    assert(fd > 0);

    u_int32_t n;
    int ret = readTimeOut(fd, &n, sizeof(n), t);
    printf("File Size = %d\n", n);
    char buffer[n];

    FILE *received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        perror("fopen");
        return;
    }

    int b = completeReadTimeout(fd, buffer, n, t);
    b = fwrite(buffer, sizeof(char), n, received_file);
    fclose(received_file);
}

ssize_t sendCustomFile(int fd, const char *filename)
{
    assert(filename != NULL);


    int file_to_send = open(filename, O_RDONLY);
    if (file_to_send == 0)
    {
        perror("fopen");
        return -1;
    }

    // Get file size
    struct stat my_file_stat;
    if (fstat(file_to_send, &my_file_stat) < 0)
    {
        perror("fstat");
        return -1;
    }

    u_int32_t file_size = my_file_stat.st_size;
    write(fd, &file_size, sizeof(file_size));
    completeSend(fd, file_to_send, file_size);

    close(file_to_send);

    return file_size;
}

