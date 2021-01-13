#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>

int8_t initializeServer(u_int16_t port_no, u_int16_t max_clients);
int acceptTimeOut(u_int16_t wait, struct sockaddr_in *client_addr);
void handleConnection(int client_no, int fd, u_int16_t timeout);
void *handleConnectionWrapper(void *args);

int serverRoutine(u_int16_t port_no, u_int16_t max_clients, u_int16_t waitClients, u_int16_t waitData);

#endif // SERVER_H
