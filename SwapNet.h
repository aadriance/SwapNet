#define _GNU_SOURCE     /* for RTLD_NEXT */
#include <sys/socket.h>
#include <dlfcn.h>


//libc functions to shadow
int  accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);
int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int listen(int socket, int backlog);
ssize_t send(int socket, const void *message, size_t length, int flags);
ssize_t recv(int socket, void *buffer, size_t length, int flags);
int     shutdown(int socket, int how);

//libc wrappers
int  libc_accept(int socket, struct sockaddr *address, socklen_t *address_len);
int libc_bind(int socket, const struct sockaddr *address, socklen_t address_len);
int libc_connect(int socket, const struct sockaddr *address, socklen_t address_len);
int libc_getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);
int libc_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int libc_listen(int socket, int backlog);
ssize_t libc_send(int socket, const void *message, size_t length, int flags);
ssize_t libc_recv(int socket, void *buffer, size_t length, int flags);
int libc_shutdown(int socket, int how);