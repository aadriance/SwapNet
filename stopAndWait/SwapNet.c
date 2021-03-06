#define _GNU_SOURCE     /* for RTLD_NEXT */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <stdarg.h>
#include <getopt.h>

#include <fcntl.h>

#include <assert.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <strings.h>
#include <sys/stat.h>

//libc wrappers for sys/sockets.h
int  libc_accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    int (*lib_accpet)(int, struct sockaddr*, socklen_t*) = (int (*)(int, struct sockaddr*, socklen_t*))dlsym(RTLD_NEXT, "accept");
    return lib_accpet(socket, address, address_len);
}
int libc_bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    int (*lib_bind)(int, const struct sockaddr *, socklen_t) = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "bind");
    return lib_bind(socket, address, address_len);
}
int libc_connect(int socket, const struct sockaddr *address, socklen_t address_len) {
    int (*lib_connect)(int, const struct sockaddr *, socklen_t) = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
    return lib_connect(socket, address, address_len);
}
int libc_getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len) {
    int (*lib_getsockopt)(int, int, int, void *, socklen_t *) = (int (*)(int, int, int, void *, socklen_t *))dlsym(RTLD_NEXT, "getsockopt");
    return lib_getsockopt(socket, level, option_name, option_value, option_len);
}
int libc_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len) {
    int (*lib_setsockopt)(int, int, int, const void *, socklen_t) = (int (*)(int, int, int, const void *, socklen_t))dlsym(RTLD_NEXT, "setsockopt");
    return lib_setsockopt(socket, level, option_name, option_value, option_len);
}
int libc_listen(int socket, int backlog) {
    int (*lib_listen)(int, int) = (int (*)(int, int))dlsym(RTLD_NEXT, "listen");
    return lib_listen(socket, backlog);
}
ssize_t libc_send(int socket, const void *message, size_t length, int flags) {
    ssize_t (*lib_send)(int, const void *, size_t, int) = (ssize_t (*)(int, const void *, size_t, int))dlsym(RTLD_NEXT, "send");
    return lib_send(socket, message, length, flags);
}
ssize_t libc_recv(int socket, void *buffer, size_t length, int flags) {
    ssize_t (*lib_recv)(int, void *, size_t, int) = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_NEXT, "recv");
    return lib_recv(socket, buffer, length, flags);
}
int libc_shutdown(int socket, int how) {
    int (*lib_shutdown)(int, int) = (int (*)(int, int))dlsym(RTLD_NEXT, "shutdown");
    return lib_shutdown(socket, how);
}
int libc_socket(int domain, int type, int protocol) {
    int (*lib_socket)(int, int, int) = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
    return lib_socket(domain, type, protocol);
}

//START STOP AND WAIT CODE

#define HEADER_SIZE 8
#define PACKET_SIZE 512
#define PAYLOAD_SIZE (PACKET_SIZE-HEADER_SIZE)
#define RETRY 10

typedef struct struct_socket_data {
    int socket;
    unsigned int addrLen;
    struct sockaddr address;
} socket_data;

typedef struct struct_socket_list {
    socket_data *data;
    struct struct_socket_list *next;
}socket_list_node;

enum Packet_Type
{
   ACK, FIN, DATA
};

typedef struct struct_packet {
        u_short seq;
        u_short packetType;
        u_short size;
        u_char payload[PAYLOAD_SIZE];
} packet;


socket_list_node *socketList = NULL;
socket_list_node *socketListTail = NULL;

socket_data *getDataForSocket(int socket) {
    socket_list_node *res_node = socketList;
    //link list search
    while(res_node != NULL && res_node->data->socket != socket) {
        res_node = res_node->next;
    }
    if(res_node == NULL) {
        return NULL;
    } else {
        return res_node->data;
    }
}

int removeConnection(int socket) {
    socket_list_node *res_node = socketList;
    socket_list_node *prev_node = NULL;
    while(res_node != NULL && res_node->data->socket != socket) {
        prev_node = res_node;
        res_node = res_node->next;
    }
    //standard linked list removal
    if(res_node != NULL) {
        if (prev_node == NULL) {
            socketList = socketList->next;
        } else if(res_node == socketListTail) {
            socketListTail = prev_node;
            socketListTail->next = NULL;
        } else {
            prev_node->next = res_node->next;
        }
        free(res_node->data);
        free(res_node);
    }
    return 0;
}

int registerConnection(int socket, struct sockaddr *address, socklen_t address_len) {
    socket_list_node *new_node = malloc(sizeof(socket_list_node));
    socket_data *data = malloc(sizeof(socket_data));
    //return failure if mallocs failed
    if (new_node <= 0 || data <= 0) {
        return -1;
    }
    //init data
    data->socket = socket;
    data->address = *address;
    data->addrLen = address_len;
    new_node->data = data;
    new_node->next = NULL;
    //link list add
    if(socketList == NULL){
        socketList = new_node;
        socketListTail = new_node;
    } else {
        socketListTail->next = new_node;
        socketListTail = new_node;
    }

    return socket;
}

void printAddr(void *address){

    struct sockaddr_in *inAddr = ((struct sockaddr_in *) address);
    //printf("Numeric: %u\n", ntohl(inAddr->sin_addr.s_addr));
    //printf("sin_port: %d\n", inAddr->sin_port);
    printf("saddr = %d, %s: %d\n", inAddr->sin_family, inet_ntoa(inAddr->sin_addr), ntohs(inAddr->sin_port));
    //printf("s_addr: %d\n", ntohl(inAddr->sin_addr.s_addr));
}

//END STOP AND WAIT CODE


//START SOCKET.H OVERRIDES

int socket(int domain, int type, int protocol) {
    if(type == SOCK_STREAM) {
        type = SOCK_DGRAM;
    }
    return libc_socket(domain, type, protocol);
}

int  accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    printf("Intercepted accept\n");
    //Establishes server connection to client
    //call setup_ack()
    //Server will:
    //  -recvfrom client the connect packet
    //  -set up sliding window
    //  -send an ACK to client
    packet getPacket, ackPacket;
    ackPacket.packetType = ACK;
    struct sockaddr_in serverAddr;
    recvfrom(socket, &getPacket, sizeof(getPacket), 0, address, address_len);
    //printAddr(address);
    //set up new socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(ntohs(((struct sockaddr_in *)address)->sin_port)  + 1);
    serverAddr.sin_addr.s_addr = ((struct sockaddr_in *)address)->sin_addr.s_addr;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    int newSocket = libc_socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(newSocket,  (const struct sockaddr *)&serverAddr, *address_len) == -1) 
    {
        perror("Bind Error");
        return -1;
    }
    //respond
    sendto(newSocket, &ackPacket, sizeof(ackPacket), 0, address, *address_len);
    //printAddr(&serverAddr);
    //store what socket to talk to in future
    return registerConnection(newSocket, address, *address_len);
}
int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    printf("Intercepted bind\n");
    //will not change for goBackN
    return libc_bind(socket, address, address_len);
}
int connect(int socket, const struct sockaddr *address, socklen_t address_len) {
    printf("Intercepted connect\n");
    //Establish client connection to server
    //call call_setup()
    //packet will contain:
    //  -window size
    //client does:
    //  -send setup packet
    //  -set up own sliding window
    packet sendPacket, ackPacket;
    sendPacket.packetType = DATA;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size = sizeof(serverStorage);
    struct sockaddr *addr = address;
    sendto(socket, &sendPacket, sizeof(sendPacket), 0, addr, address_len);
    recvfrom(socket, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&serverStorage, &addr_size);
    if(ackPacket.packetType == ACK) {
        //printAddr(&serverStorage);
        return registerConnection(socket, (struct sockaddr *)&serverStorage, addr_size) == -1 ? -1:0;;
    }
    return -1;
}
int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len) {
    printf("Intercepted getsockopt\n");
    if(level == IPPROTO_TCP) {
        level = IPPROTO_UDP;
    }
    //won't change
    return libc_getsockopt(socket, level, option_name, option_value, option_len);
}
int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len) {
    printf("Intercepted setsocketopt\n");
    if(level == IPPROTO_TCP) {
        level = IPPROTO_UDP;
    }
    //won't change
    return libc_setsockopt(socket, level, option_name, option_value, option_len);
}
int listen(int socket, int backlog) {
    printf("Intercepted listen\n");
    //server opens port and waits for cleint to connect, then server will accept
    //call start_state()
    //server will:
    //  -open UDP port
    // Connected to server before, close and reopen connection
    // okay actually maybe not, open UDP sockets already do this
    return 0;
}
ssize_t send(int socket, const void *message, size_t length, int flags) {
    printf("Intercepted send\n");
    //sender:
    //  -While there is data to send:
    //      - Fill a packet with data and sendto()
    //      - use select and recvfrom() to timeout
    //      - if timeout
    //          - re transmit up to 10(?) times
    //  - if all data is sent, send fin packet
    //  - use select and recvfrom() to timeout for fin ack
    socket_data *socketInfo = getDataForSocket(socket);
    //printAddr(&socketInfo->address);
    size_t toSend = length;
    packet sendPacket;
    int seq = 0;
    void *messageLoc = message;
    while(toSend > 0) {
        //figure out how much we are goign to send
        size_t packetSize = toSend;
        if(toSend > PACKET_SIZE) {
            packetSize = PAYLOAD_SIZE;
        }

        //fill packet with data
        sendPacket.packetType = DATA;
        sendPacket.size = packetSize;
        sendPacket.seq = seq;
        memcpy(&(sendPacket.payload), messageLoc, packetSize );

        int attempts = 0;
        while(attempts < 10){
            fd_set rfds;
            struct timeval tv;
            int retval = 1;
            FD_ZERO(&rfds);
            FD_SET(socket, &rfds);
            //sendto
            sendto(socket, &sendPacket, sizeof(sendPacket), 0,
                &socketInfo->address, socketInfo->addrLen);
            /* Wait up to five seconds. */
            tv.tv_sec = 0;
            tv.tv_usec = 500 * 1000;
            //timeout waiting for data
            retval = select(socket + 1, &rfds, NULL, NULL, &tv);
            if (retval == -1) {
                perror("select()");
            }
            else if (retval) {
                packet ackPacket;
                recvfrom(socket, &ackPacket, sizeof(ackPacket),
                    0, NULL, NULL);
                //printf("Got response\n");
                if(ackPacket.packetType == ACK) {
                    //printf("ACK\n");
                    break;
                }
            } else {
                //printf("No ACK\n");
            }
            attempts++;
        }
        //Nevet got an ACK
        if(attempts >= 9) {
            return -1;
        }

        //update loop parameters
        seq++;
        messageLoc += packetSize;
        toSend -= packetSize;
    }
    //send fin
    sendPacket.packetType = FIN;
    //printf("FIN\n");
    sendto(socket, &sendPacket, sizeof(sendPacket), 0,
        &socketInfo->address, socketInfo->addrLen);
    return length - toSend;
}
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
    printf("Intercepted recv\n");
    //recving:
    // - while data < length || packetType != fin
    //      - recvfrom() to get data, use select() to timeout
    //      - if no timeout
    //          - add data to buffer
    //          - send ack
    //          - repeat
    //      - if timeout
    //          - try again, up to 10(?) times
    // -if packet if fin
    //      - send ack
    socket_data *socketInfo = getDataForSocket(socket);
    //printAddr(&socketInfo->address);
    size_t gatheredAmount = 0;
    int seq = 0;
    void *writeLoc = buffer;
    packet ackPacket;
    ackPacket.packetType = ACK;
    ackPacket.size = 0;
    while(gatheredAmount < length){
        //attempt 10 times to get data
        int attempts = 0;
        while(attempts < 10){
            fd_set rfds;
            struct timeval tv;
            int retval = 1;
            FD_ZERO(&rfds);
            FD_SET(socket, &rfds);

            /* Wait up to five seconds. */
            tv.tv_sec = 0;
            tv.tv_usec = 600 * 1000;
            //timeout waiting for data
            retval = select(socket + 1, &rfds, NULL, NULL, &tv);
            if (retval == -1) {
                perror("select()");
            }
            else if (retval) {
                packet recvPacket;
                recvfrom(socket, &recvPacket, sizeof(recvPacket),
                    0, NULL, NULL);
                //printf("Got data, %d\n", recvPacket.seq);
                if(recvPacket.packetType == FIN) {
                    //if final packet, return
                    return gatheredAmount;
                } else if(recvPacket.packetType == DATA) {
                    //if data packet check sequence
                    if(recvPacket.seq == seq){
                        //copy in data
                        memcpy(writeLoc, &(recvPacket.payload), recvPacket.size );
                        gatheredAmount += recvPacket.size;
                        writeLoc += recvPacket.size;
                        seq++;
                        sendto(socket, &ackPacket, sizeof(ackPacket), 0,
                         &socketInfo->address, socketInfo->addrLen);
                    } else if(recvPacket.seq == seq) {
                        //if we got the last paket again, resend ack
                        sendto(socket, &ackPacket, sizeof(ackPacket), 0,
                            &socketInfo->address, socketInfo->addrLen);
                    }

                }
            } else {
                //printf("No data to recv\n");
            }
            attempts++;
        }
        //Nevet got a data
        if(attempts >= 9) {
            return -1;
        }
        seq++;
    }
    return gatheredAmount;
}
int     shutdown(int socket, int how) {
    printf("Intercepted shutdown\n");
    //close connection.  Clean up connection state data.  Un allocate sliding windows, etc.
    removeConnection(socket);
    return libc_shutdown(socket, how);
}

//END SOCKET.H OVERRIDES