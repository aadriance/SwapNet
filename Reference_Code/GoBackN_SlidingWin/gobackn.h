/* Code provided by Prof. Hugh Smith for CPE 464 (Networks) at
 * Cal Poly San Luis Obispo.
 */
#ifndef _GOBACKN_H_
#define _GOBACKN_H_

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

#define MAX_LEN          1500
#define SIZE_OF_BUF_SIZE 4
#define START_SEQ_NUM    1
#define MAX_TRIES        10
#define LONG_TIME        10
#define SHORT_TIME       1
//#define DEBUG

#pragma pack(1)

typedef struct header Header;
typedef struct datapkt DataPacket;

struct header
{
   uint32_t seq_num;
   uint16_t checksum;
   uint8_t flag;
};

/* For use with the server's buffer */
struct datapkt
{
   uint32_t inWindow;
   uint8_t packet[MAX_LEN];
};

enum flag
{
   NO_FLAG, SETUP, ACK_SETUP, DATA, EMPTY, RR, REJ, FNAME, FNAME_OK, FNAME_BAD, END_OF_FILE, EOF_ACK, ACK, CRC_ERROR = -1
};

int32_t send_buf(uint8_t *buf, uint32_t len, Connection *connection,
                     uint8_t flag, uint32_t seq_num, uint8_t *packet);
int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet);
int32_t recv_buf(uint8_t *buf, int32_t len, int32_t recv_sk_num,
                     Connection *connection, uint8_t *flag, uint32_t *seq_num);
int retrieveHeader(char *data_buf, int recv_len, uint8_t *flag, uint32_t *seq_num);
void printPkt(uint8_t *buf, int32_t buf_len);
int processSelect(Connection *client, int *retryCount,
                     int selectTimeoutState, int dataReadyState, int doneState);

#endif
