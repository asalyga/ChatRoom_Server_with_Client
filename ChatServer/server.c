#include "server.h"
#include "readtimeout.h"
#include "systemsettings.h"
#include <pthread.h>

static int server_fd = -1;
static u_int16_t connected_client_count = 0,
max_clients_connected = 0;
static int client_fd[MAX_CLIENTS] = {-1};
static char usernames[MAX_CLIENTS][256];

static pthread_mutex_t lock;

typedef struct Connection_Struct
{
    int fd, client_no;
    u_int16_t timeout;
}Connection_Struct;

// creates a BSD socket, binds it to specified port_no and starts listening.
// returns 0 in case of no error on initialization, otherwise prints error message and returns -1
int8_t initializeServer(u_int16_t port_no, u_int16_t max_clients)
{
    // Initialize connected clients counter
    connected_client_count 	= 0;
    max_clients_connected = max_clients;

    // create server socket
    server_fd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, IPPROTO_TCP);
    if(server_fd < 0) {
        perror("socket");
        return -1;
    }

    int val = 1;
    // allow reusing of a non active port number
    int Status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (Status == -1){
        perror("setsockopt");
    }
    Status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
    if (Status == -1){
        perror("setsockopt");
    }

    // bind server socket to a specified port, and make it ready to listen from any address
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port_no);
    saddr.sin_addr.s_addr = INADDR_ANY;
    if(bind( server_fd, ((struct sockaddr*)&saddr), sizeof(struct sockaddr_in) ) < 0) {
        perror("bind");
        server_fd = -1;
        return -1;
    }
    if(listen(server_fd, max_clients) < 0) {
        perror("listen");
        server_fd = -1;
        return -1;
    }

    printf("Successful server initialization\n");
    return 0;
}

// Accept client connection requested in a wait seconds
// return file descriptor to be used for connection with that client
// update client_addr with the address of client
int acceptTimeOut(u_int16_t wait, struct sockaddr_in *client_addr)
{
    if(connected_client_count < max_clients_connected) {
        socklen_t peer_addr_size = sizeof(struct sockaddr);
        struct timeval t_sec;
        int ret;

        // see "man select" or "man 2 select_tut" for details
        fd_set readfds, writefds, exceptfds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        // using only accept will block forever. using in combination with select this can return if main gui quits, (it will check if main gui has quit after interval sec)
        // using accept with SOCK_NOBLOCK in a while-loop will take up too much of unnecessary resources

        t_sec.tv_sec = wait;
        t_sec.tv_usec = 0;
        FD_SET(server_fd, &readfds);
        ret = select(server_fd+1, &readfds, &writefds, &exceptfds, &t_sec);

        if(ret < 0) {
            perror("select");
            return -1;
        }
        else if(ret == 0) {
            printf("timeout in acceptTimeout\n");
            return -1;
        }
        else {
            if (FD_ISSET(server_fd, &readfds)) {
                int cfd = accept(server_fd, (struct sockaddr*)(client_addr), &peer_addr_size);
                if (cfd < 0) {
                    perror("accept");
                    return -1;
                }
                else {
                    printf("Client Fd = %d\n",cfd );
                    client_fd[connected_client_count++] = cfd; // increment the client count
                    return cfd;
                }
            }
            else {
                printf("Invalid read detect in acceptTimeout\n");
                return -1;
            }
        }
    }
    else {
        printf(" maximum clients already connected to server\n");
        return -1;
    }
}

void handleConnection(int client_no, int fd, u_int16_t timeout)
{
    int ret = 1;
    printf("Connection Handler Thread Invoked with fd = %d\n", fd);

    // Receive Username
    u_int32_t bytes;
    ret = readTimeOut(fd, &bytes, sizeof(bytes), timeout);
    if(ret > 0 && ret < 256) {
        ret = readTimeOut(fd, &usernames[client_no][0], bytes, timeout);
        usernames[client_no][bytes] = '\0';
        printf("User name: %s\n", usernames[client_no]);


        char tx_buf[256] = {' '};
        bytes = strlen(tx_buf);
        write(fd, &bytes, sizeof(bytes));
        write(fd, tx_buf, strlen(tx_buf));
    }
    else
        return;

    while(ret) {
        u_int32_t bytes_expected;
        ret = readTimeOut(fd, &bytes_expected, sizeof(bytes_expected), timeout);
        if(ret > 0) {
            char rx_buf[bytes_expected+1];
            ret = readTimeOut(fd, rx_buf, bytes_expected, timeout);
            if(ret > 0) {
                if(rx_buf[0] == '1') {
                    rx_buf[bytes_expected] = '\0';
                    pthread_mutex_lock(&lock);
                    printf("%s::   %s\n", usernames[client_no], rx_buf);

                    char tx_buf[512];
                    sprintf(tx_buf, "%s::%s\n", usernames[client_no], rx_buf);
                    bytes_expected = strlen(tx_buf);
                    for(int i = 0; i < connected_client_count; ++i) {
                        write(client_fd[i], &bytes_expected, sizeof(bytes_expected));
                        write(client_fd[i], tx_buf, bytes_expected);
                    }
                    printf("Echo Sent\n");
                    pthread_mutex_unlock(&lock);
                }
                else if(rx_buf[0] == '2') {
                    rx_buf[bytes_expected] = '\0';
                    printf("%s::   %s\n", usernames[client_no], rx_buf);
                    char filepath[512];
                    memcpy(filepath, &rx_buf[1], strlen(rx_buf) - 1);
                    printf("filepath = %s", filepath);
                    recvFile(fd, filepath, 1000);

                    char tx_buf[512];
                    sprintf(tx_buf, "%s::%s\n", usernames[client_no], rx_buf);
                    bytes_expected = strlen(tx_buf);

                    pthread_mutex_lock(&lock);
                    for(int i = 0; i < connected_client_count; ++i) {
                        write(client_fd[i], &bytes_expected, sizeof(bytes_expected));
                        write(client_fd[i], tx_buf, bytes_expected);
                    }
                    printf("Echo Sent\n");
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        else
            break;
    }

    close(fd);
}

void *handleConnectionWrapper(void *args)
{
    Connection_Struct *in = (Connection_Struct *)args;
    handleConnection(in->client_no, in->fd, in->timeout);
    return NULL;
}

int serverRoutine(u_int16_t port_no, u_int16_t max_clients, u_int16_t waitClients, u_int16_t waitData)
{
    pthread_t threads[max_clients];
    u_int16_t thread_counter = 0;
    struct Connection_Struct conn_prop[max_clients];
    int ret = initializeServer(port_no, max_clients);
    if(ret == -1) {
        printf("Server Initialization Error\n");
        return -1;
    }

    do {
        struct sockaddr_in caddr;
        printf("Waiting for Clients\n");
        ret = acceptTimeOut(waitClients, &caddr);
        if(ret == -1) {
            printf("Error in Accept\n");
            return -1;
        }

        printf("Client connected. Invoking Thread Right Now\n");
        conn_prop[thread_counter].fd = client_fd[thread_counter];
        conn_prop[thread_counter].client_no = connected_client_count - 1;
        conn_prop[thread_counter].timeout = waitData;
        pthread_create(&threads[thread_counter], NULL, handleConnectionWrapper, (void*)(&conn_prop[thread_counter]));
        ++thread_counter;
    }while (connected_client_count < max_clients);

    for(uint i = 0; i < thread_counter; ++i)
        pthread_join(threads[i], NULL);

    return 0;
}
