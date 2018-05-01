/* Code provided by Prof. Hugh Smith for CPE 464 (Networks) at
 * Cal Poly San Luis Obispo.
 */
 /* Used for network functions */
#ifndef _NETWORKS_H_
#define _NETWORKS_H_

//#define DEBUG
#define MAX_LEN 1500

enum SELECT
{
   SET_NULL, NOT_NULL
};

typedef struct connection Connection;

struct connection
{
   int32_t sk_num;
   struct sockaddr_in remote;
   uint32_t len;
};

int32_t udp_server(int portNumber);
int32_t udp_client_setup(char *hostname, uint16_t port_num, Connection *connection);
int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds,
                        int32_t set_null);
int32_t safeSend(uint8_t * packet, uint32_t len, Connection * connection);
int32_t safeRecv(int recv_sk_num, char *data_buf, int len, Connection * connection);

#endif
