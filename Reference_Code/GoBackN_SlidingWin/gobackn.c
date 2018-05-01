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
#include "gobackn.h"

#include "cpe464.h"

int32_t send_buf(uint8_t *buf, uint32_t len, Connection *connection,
                     uint8_t flag, uint32_t seq_num, uint8_t *packet)
{
   int32_t sentLen = 0;
   int32_t sendingLen = 0;

   /* Sets up packet (seq#, crc, flag, data) */
   if (len > 0)
   {
      memcpy(&packet[sizeof(Header)], buf, len);
   }
   sendingLen = createHeader(len, flag, seq_num, packet);
   sentLen = safeSend(packet, sendingLen, connection);

   return sentLen;
}

int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet)
{
   /* Creates regular header (puts in pkt)...includes
      the seq num, flag, and checksum */

   Header *aHeader = (Header *) packet;
   uint16_t checksum = 0;

   #ifdef DEBUG
      printf("Seq. Num. in Header: %d\n", seq_num);
   #endif
   seq_num = htonl(seq_num);
   memcpy(&(aHeader->seq_num), &seq_num, sizeof(seq_num));

   aHeader->flag = flag;

   memset(&(aHeader->checksum), 0, sizeof(checksum));
   checksum = in_cksum((unsigned short *) packet, len + sizeof(Header));
   memcpy(&(aHeader->checksum), &checksum, sizeof(checksum));

   return len + sizeof(Header);
}

int32_t recv_buf(uint8_t *buf, int32_t len, int32_t recv_sk_num,
                     Connection *connection, uint8_t *flag, uint32_t *seq_num)
{
   char data_buf[MAX_LEN];
   int32_t recv_len = 0;
   int32_t dataLen = 0;

   recv_len = safeRecv(recv_sk_num, data_buf, len, connection);
   #ifdef DEBUG
      printf("in recv_buf, recv_len: %d\n", recv_len);
   #endif

   dataLen = retrieveHeader(data_buf, recv_len, flag, seq_num);

   /* dataLen could be -1 if crc error or 0 if no data */
   if (dataLen > 0)
      memcpy(buf, &data_buf[sizeof(Header)], dataLen);

   return dataLen;
}

int retrieveHeader(char *data_buf, int recv_len, uint8_t *flag, uint32_t *seq_num)
{
   Header *aHeader = (Header *) data_buf;
   int returnValue = 0;

   if (in_cksum((unsigned short *)data_buf, recv_len) != 0)
   {
      returnValue = CRC_ERROR;
   }
   else
   {
      *flag = aHeader->flag;
      memcpy(seq_num, &(aHeader->seq_num), sizeof(aHeader->seq_num));
      *seq_num = ntohl(*seq_num);
      #ifdef DEBUG
         printf("aHeader's retrieved seq->num: %d\n", *seq_num);
      #endif
      returnValue = recv_len - sizeof(Header);
   }
   return returnValue;
}

/* printPkt prints the buffer provided into a byte-by-byte printout.
 */
void printPkt(uint8_t *buf, int32_t buf_len)
{
   // Print out all the byte-by-byte contents of the packet.
   int i;
   for (i = 0; i < buf_len; i++) {
      if (i % 8 == 0) {
         printf("\n");
      }
      printf("%02x ", buf[i]);
   }
   printf("\n");
}

int processSelect(Connection *client, int *retryCount,
                     int selectTimeoutState, int dataReadyState, int doneState)
{
   /* Returns:
    * doneState if calling this funtion exceeds MAX_TRIES
    * selectTimeoutState if the select times out without receiving anything
    * dataReadyState if select() returns indicating that data is ready for read
    */
   int returnValue = dataReadyState;

   (*retryCount)++;
   if (*retryCount > MAX_TRIES) {
      printf("Sent data %d times, no ACK, recipient is probably gone - so I'm terminating\n",
         MAX_TRIES);
      returnValue = doneState;
   }
   else {
      if (select_call(client->sk_num, SHORT_TIME, 0, NOT_NULL) == 1) {
         *retryCount = 0;
         returnValue = dataReadyState;
      }
      else {
         returnValue = selectTimeoutState;
      }
   }
   return returnValue;
}
