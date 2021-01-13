#ifndef CLIENT_H
#define CLIENT_H



#include <iostream>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

#include <assert.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>

using namespace std;

void ClientTest();

typedef uint16_t timeout_sec;

#define COMM_FAILURE	-1
#define COMM_SUCCESS	 0

#define PORT_NO			 6000
#define LISTEN_BACKLOG	 1

/** Objects of this class can only be passed as reference
 * To avoid the problem that passing by value will create a copy and on exit closes connection
 * Inherently objects of all classes that uses objects of this class cannot be passed as value too
 */
class TCP_Client
{
private:
    int fd;
    ssize_t read_t(int fd, void *buf, size_t count, timeout_sec t);

public:
    TCP_Client();
    TCP_Client(const TCP_Client& ) = delete; 		// disable passing by value/ copy constructor
    TCP_Client operator=(const TCP_Client& ) = delete;	// disable equating to other objects

    ~TCP_Client(void);

    // try to connect for tout secs, retries after every "retry" secs until tout
    int connect_t(const char* server_addr ="127.0.0.1",
                  uint16_t port_no = PORT_NO,
                  timeout_sec tout = 6, timeout_sec retry = 2);
    // file descriptor is read only
    int get_fd(void);
    // Handshake
    int HandleConn();
    // complete read with timeout
    ssize_t cread_t(int fd, void *dst, size_t n, timeout_sec t);
    // client complete read with timeout
    ssize_t ccread_t(void *buf, size_t n, timeout_sec t);
    // client write
    ssize_t cwrite(void *buf, size_t n);

    ssize_t completeSend(int fd_in, size_t n);
    ssize_t recvFile(const char *filename, size_t n, u_int16_t t);
    ssize_t sendCustomFile(const char *filename);
};


#endif // CLIENT_H
