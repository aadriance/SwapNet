#include "libcFunctions.h"
#include <stdio.h>

int  accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    printf("Intercepted accept\n");
    return libc_accept(socket, address, address_len);
}
int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    printf("Intercepted bind\n");
    return libc_bind(socket, address, address_len);
}
int connect(int socket, const struct sockaddr *address, socklen_t address_len) {
    printf("Intercepted connect\n");
    return libc_connect(socket, address, address_len);
}
int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len) {
    printf("Intercepted getsockopt\n");
    return libc_getsockopt(socket, level, option_name, option_value, option_len);
}
int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len) {
    printf("Intercepted setsocketopt\n");
    return libc_setsockopt(socket, level, option_name, option_value, option_len);
}
int listen(int socket, int backlog) {
    printf("Intercepted listen\n");
    return libc_listen(socket, backlog);
}
ssize_t send(int socket, const void *message, size_t length, int flags) {
    printf("Intercepted send\n");
    return libc_send(socket, message, length, flags);
}
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
    printf("Intercepted recv\n");
    return libc_recv(socket, buffer, length, flags);
}
int     shutdown(int socket, int how) {
    printf("Intercepted shutdown\n");
    return libc_shutdown(socket, how);
}