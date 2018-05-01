/* Code provided by Prof. Hugh Smith for CPE 464 (Networks) at
 * Cal Poly San Luis Obispo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "cpe464.h"

int32_t udp_server(int portNumber)
{
   int sk = 0;                    // Socket descriptor
   struct sockaddr_in local;      // Socket address for us
   uint32_t len = sizeof(local);  // Length of local address

   // Create the socket
   if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("socket");
      exit(-1);
   }

   // SET UP the socket
   local.sin_family = AF_INET;         // Internet Family
   local.sin_addr.s_addr = INADDR_ANY; // Wild Card machine address
   local.sin_port = htons(portNumber);

   // Bind the name/address to the port.
   if (bindMod(sk, (struct sockaddr *)&local, sizeof(local)) < 0) {
      perror("udp_server, bind");
      exit(-1);
   }

   // Get the port name and print int32_t out
   getsockname(sk, (struct sockaddr *) &local, &len);
   printf("Using Port #: %d\n", ntohs(local.sin_port));

   return sk;
}

int32_t udp_client_setup(char *hostname, uint16_t port_num, Connection *connection)
{
   /* Returns pointer to a sockaddr_in that it created or NULL if host not found
    * Also passes back the socket number in sk */
   struct hostent *hp = NULL;   // Address of remote host

   connection->sk_num = 0;
   connection->len = sizeof(struct sockaddr_in);
   
   // Create the socket
   if ((connection->sk_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("udp_client_setup, socket");
      exit(-1);
   }

   // Addressing family
   connection->remote.sin_family = AF_INET;

   // Address of remote host stored
   hp = gethostbyname(hostname);

   if (hp == NULL) {
      printf("Host not found: %s\n", hostname);
      return -1;
   }

   memcpy(&(connection->remote.sin_addr), hp->h_addr, hp->h_length);

   // Get port used on remote side and store
   connection->remote.sin_port = htons(port_num);

   return 0;
}

int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null)
{
   fd_set fdvar;
   struct timeval aTimeout;
   struct timeval *timeout = NULL;

   if (set_null == NOT_NULL) {
      aTimeout.tv_sec = seconds;   
      aTimeout.tv_usec = microseconds;  //: Set timeout (in micro-second)
      timeout = &aTimeout;
   }

   FD_ZERO(&fdvar);
   FD_SET(socket_num, &fdvar);

   if (select(socket_num + 1, (fd_set *) &fdvar, NULL, NULL, timeout) < 0) {
      perror("select");
      exit(-1);
   }

   if (FD_ISSET(socket_num, &fdvar)) {
      return 1;
   }
   else {
      return 0;
   }
}

int32_t safeSend(uint8_t *packet, uint32_t len, Connection *connection)
{
   int send_len = 0;

   if ((send_len = sendtoErr(connection->sk_num, packet, len, 0,
            (struct sockaddr *) &(connection->remote), connection->len)) < 0) {
      perror("in send_buf(), sendto() call");
      exit(-1);
   }

   return send_len;
}

int32_t safeRecv(int recv_sk_num, char *data_buf, int len, Connection *connection)
{
   int recv_len = 0;
   uint32_t remote_len = sizeof(struct sockaddr_in);

   recv_len = recvfrom(recv_sk_num, data_buf, len, 0,
         (struct sockaddr *) &(connection->remote), &remote_len);
   if (recv_len < 0) {
      perror("recv_buf, recvfrom");
      exit(-1);
   }

   connection->len = remote_len;
   return recv_len;
}
