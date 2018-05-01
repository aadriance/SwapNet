/* Code provided by Prof. Hugh Smith for CPE 464 (Networks) at
 * Cal Poly San Luis Obispo.
 *
 * Revisions done by Spencer Chang to a Go-Back-N Algorithm
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

//#define DEBUG
typedef enum State STATE;

enum State
{
   DONE, CALL_SETUP, FILENAME, RECV_DATA, FILE_OK, START_STATE
};

STATE start_state(char **argv, Connection *server);
STATE call_setup(char **argv, Connection *server, uint32_t win_size, uint32_t buf_size);
STATE filename(char *fname, Connection *server);
STATE recv_data(int32_t output_file, Connection *server);
STATE file_ok(int *output_file_fd, char *outputFileName);
void check_args(int argc, char **argv);

int main(int argc, char **argv)
{
   Connection server;
   int32_t output_file_fd = 0;
   STATE state = START_STATE;

   check_args(argc, argv);

   sendtoErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
   server.sk_num = 0;

   while (state != DONE) {
      switch(state)
      {
         case START_STATE:
            #ifdef DEBUG
               printf("Start State\n");
               printf("server socket number: %d\n", server.sk_num);
            #endif
            state = start_state(argv, &server);
            break;

         case CALL_SETUP:
            state = call_setup(argv, &server, atoi(argv[3]), atoi(argv[4]));
            break;

         case FILENAME:
            state = filename(argv[1], &server);
            break;

         case FILE_OK:
            state = file_ok(&output_file_fd, argv[2]);
            break;

         case RECV_DATA:
            state = recv_data(output_file_fd, &server);
            break;

         case DONE:
            break;

         default:
            printf("I'm in the default state....how erroneous!\n");
            state = DONE;
            break;
      }
   }
   return 0;
}

STATE start_state(char **argv, Connection *server)
{
   // Returns CALL_SETUP if no error, else DONE
   STATE returnValue = CALL_SETUP;

   // Connected to server before, close and reopen connection
   if (server->sk_num > 0) {
      #ifdef DEBUG
         printf("Connection already exists; closing and reopening connection\n");
      #endif
      printf("server socket number: %d\n", server->sk_num);
      close(server->sk_num);
   }
   
   if (udp_client_setup(argv[6], atoi(argv[7]), server) < 0) {
      #ifdef DEBUG
         printf("Could not connect to server\n");
      #endif
      returnValue = DONE;
   }
   else {
      #ifdef DEBUG
         printf("Established connection with server\n");
      #endif
      returnValue = CALL_SETUP;
   }
   return returnValue;
}

/* This state was a litle tricky, because what if packets are dropped or seen as
      CRC_ERROR? */
STATE call_setup(char **argv, Connection *server, uint32_t win_size, uint32_t buf_size)
{
   STATE ns = START_STATE;
   uint8_t flag = 0;
   uint32_t seq_num = 0;
   uint8_t buf[MAX_LEN];
   uint8_t pkt[MAX_LEN];
   uint32_t len = 0;
   int32_t recv_len;
   static int retry = 0;
   
   // Prepare our buffer for transmitting window and buffer sizes
   #ifdef DEBUG
      printf("Buffer Size: %u\n", buf_size);
      printf("Window Size: %u\n", win_size);
   #endif
   win_size = htonl(win_size);
   buf_size = htonl(buf_size);
   memcpy(buf, &win_size, sizeof(uint32_t));
   len += sizeof(uint32_t);
   memcpy(buf + len, &buf_size, sizeof(uint32_t));
   len += sizeof(uint32_t);

   // Send the actual packet out to the server after packing it up
   send_buf(buf, len, server, SETUP, 0, pkt);
   
   // Now wait for a response...
   ns = processSelect(server, &retry, CALL_SETUP, FILENAME, DONE);
   if (ns == FILENAME) {
      recv_len = recv_buf(pkt, MAX_LEN, server->sk_num, server, &flag, &seq_num);
      /* If there are any bit flips, we must return to the setup state */
      if (recv_len == CRC_ERROR) {
         ns = CALL_SETUP;
      }
      else {
         ns = FILENAME;
         #ifdef DEBUG
            printf("Call setup complete.\n");
         #endif
      }
   }
   return ns;
}

STATE filename(char *fname, Connection *server)
{
   // Send filename and get response
   // Return START_STATE if no reply (CRC Err or lost pkt), DONE if bad filename,
   //   FILE_OK otherwise
   int returnValue = FILENAME;
   uint8_t packet[MAX_LEN];
   uint8_t buf[MAX_LEN];
   uint8_t flag = 0;
   uint32_t seq_num = 0;
   int32_t fname_len = strlen(fname) + 1;
   int32_t recv_check = 0;
   static int retryCount = 0;

   // Put in buffer size and filename
   memcpy(buf, fname, fname_len);
   send_buf(buf, fname_len, server, FNAME, seq_num, packet);

   // This is a pretty slick transition to RECV_DATA (either DATA or FILE_OK comes in)
   returnValue = processSelect(server, &retryCount, START_STATE, FILE_OK, DONE);
   if (returnValue == FILE_OK) {
      /* Here, we double check the packet we recv'd in the processSelect */
      recv_check = recv_buf(packet, MAX_LEN, server->sk_num, server, &flag, &seq_num);

      /* Checking for bit flips */
      if (recv_check == CRC_ERROR || flag == ACK_SETUP) {
         returnValue = FILENAME;
      }
      else if (flag == FNAME_BAD) {
         printf("Error during file open of %s on server.", fname);
         printf(" This could happen if the file does not exist on the server");
         printf(" or you do not have read acces to the file.\n");
         returnValue = DONE;
      }
   }
   return returnValue;
}

STATE recv_data(int32_t output_file, Connection *server)
{
   uint32_t seq_num = 0;
   uint8_t flag = 0;
   int32_t data_len = 0;
   uint8_t buf[MAX_LEN];
   uint8_t data_buf[MAX_LEN];
   uint8_t packet[MAX_LEN];
   static uint32_t expected_seq_num = START_SEQ_NUM;
   static uint32_t rcopy_seq_num = START_SEQ_NUM + 1;
   static int sentRej = 0;

   if (select_call(server->sk_num, LONG_TIME, 0, NOT_NULL) == 0) {
      printf("Timeout after 10 seconds, server is probably gone.\n");
      return DONE;
   }

   data_len = recv_buf(data_buf, MAX_LEN, server->sk_num, server, &flag, &seq_num);

   /* Do state RECV_DATA again if CRC Error, silence on rcopy end */
   if (data_len == CRC_ERROR) {
      #ifdef DEBUG
         printf("CRC_ERROR; rcopy remains silent.\n");
      #endif
      return RECV_DATA;
   }
   
   if (flag == END_OF_FILE) {
      send_buf(packet, 1, server, EOF_ACK, 0, packet);
      #ifdef DEBUG
         printf("File done\n");
      #endif
      return DONE;
   }

   // Not EOF? Then it must be DATA...but what kind of data is it? Right? Wrong?
   #ifdef DEBUG
      printf("***RCOPY ::: Seq Num Recv'd: %d --- Expected: %d\n", seq_num, expected_seq_num);
   #endif
   if (seq_num == expected_seq_num) {
      /* This data is right on target */
      seq_num = htonl(seq_num + 1);    // Put the sequence number we are ready to recv
      memcpy(buf, &seq_num, sizeof(uint32_t));
      send_buf(buf, sizeof(uint32_t), server, RR, rcopy_seq_num, packet);
      expected_seq_num++;
      write(output_file, &data_buf, data_len);
      sentRej = 0;   // Clear rcopy to send REJ's anew
   }
   else if (seq_num < expected_seq_num) {
      /* This data is wrong, but rcopy receives data we already wrote. */
      seq_num = htonl(expected_seq_num);    // Put the sequence number we are ready to recv
      memcpy(buf, &seq_num, sizeof(uint32_t));
      send_buf(buf, sizeof(uint32_t), server, RR, rcopy_seq_num, packet);
      sentRej = 0;
   }
   else if (seq_num > expected_seq_num && !sentRej) {
      /* This data is wrong, and we need to tell the server to go back */
      seq_num = htonl(expected_seq_num);    // Put the sequence number we are ready to recv
      memcpy(buf, &seq_num, sizeof(uint32_t));
      send_buf(buf, sizeof(uint32_t), server, REJ, rcopy_seq_num, packet);
      sentRej = 1;   // Stop rcopy from sending multiple REJ's, regardless of CRC err's or lost pkt's
   }
   rcopy_seq_num++;

   return RECV_DATA;
}

STATE file_ok(int *output_file_fd, char *outputFileName)
{
   STATE returnValue = DONE;

   if ((*output_file_fd = open(outputFileName, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0) {
      perror("File open error");
      exit(-1);
   }
   else {
      returnValue = RECV_DATA;
   }
   return returnValue;
}

void check_args(int argc, char **argv)
{
   if (argc != 8) {
      printf("Usage: %s fromFile toFile window-size buffer_size error_rate hostname port\n", argv[0]);
      exit(-1);
   }

   if (strlen(argv[1]) > 100) {
      printf("FROM filename too long; needs to be less than 1000 and is %lu\n",
         strlen(argv[1]));
      exit(-1);
   }

   if (strlen(argv[2]) > 100) {
      printf("TO filename too long; needs to be less than 1000 and is %lu\n",
         strlen(argv[2]));
      exit(-1);
   }

   if (atoi(argv[3]) < 1) {
      printf("Window size needs to be greater than or equal to 1 and is %d\n", atoi(argv[3]));
      exit(-1);
   }

   // Put Window size thing here
   if (atoi(argv[4]) < 400 || atoi(argv[4]) > 1493) {
      printf("Buffer size needs to be between 400 and 1493 and is %d\n", atoi(argv[4]));
      exit(-1);
   }

   if (atoi(argv[5]) < 0 || atoi(argv[5]) >= 1) {
      printf("Error rate needs to be between 0 and less than 1 and is %d\n", atoi(argv[5]));
      exit(-1);
   }
}
