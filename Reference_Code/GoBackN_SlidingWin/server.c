/* Code provided by Prof. Hugh Smith for CPE 464 (Networks) at
 * Cal Poly San Luis Obispo.
 * 
 * Revisions made by Spencer Chang to a Go-Back-N algorithm
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
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
//#define DEBUG_PKT

typedef enum State STATE;

enum State
{
   START, DONE, SETUP_ACK, FILENAME, QUICK_WAIT, PROCESS_INCOMING, WAIT_ON_RR, TIMEOUT_ON_RR, SEND_DATA, WAIT_ON_EOF_ACK,
   TIMEOUT_ON_EOF_ACK
};

void process_server(int server_sk_num);
void process_client(int32_t server_sk_num, uint8_t *buf, int32_t recv_len,
   Connection *client);
STATE setup_ack(Connection *client, uint8_t *buf, int32_t *buf_len, uint32_t *win_size,
   uint32_t *buf_size, uint32_t seq_num);
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file,
   DataPacket **window, uint32_t win_size);
STATE quick_wait(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num);
STATE process_incoming(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num);
STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file,
   DataPacket *window, int32_t win_size, int32_t buf_size, uint32_t *seq_num, uint32_t lowFrame);
STATE timeout_on_rr(Connection *client, uint8_t *packet, int32_t packet_len);
STATE timeout_on_eof_ack(Connection *client, uint8_t *packet, int32_t packet_len);
STATE wait_on_rr(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num, uint8_t *pkt, int32_t *pkt_len);
STATE wait_on_eof_ack(Connection *client);
int processArgs(int argc, char **argv);

int main(int argc, char **argv)
{
   int32_t server_sk_num = 0;
   int portNumber = 0;

   portNumber = processArgs(argc, argv);
   sendtoErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

   /* set up main server port */
   server_sk_num = udp_server(portNumber);

   process_server(server_sk_num);
   return 0;
}

void process_server(int server_sk_num)
{
   pid_t pid = 0;
   int status = 0;
   uint8_t buf[MAX_LEN];
   Connection client;
   uint8_t flag = 0;
   uint32_t seq_num = 0;
   int32_t recv_len = 0;

   // Get new client connection, fork() child, parent processes waits
   // Currently only has one client process
   while(1)
   {
      // Block waiting for a new client
      if (select_call(server_sk_num, LONG_TIME, 0, SET_NULL) == 1) {
         recv_len = recv_buf(buf, MAX_LEN, server_sk_num, &client, &flag, &seq_num);
         if (recv_len != CRC_ERROR) {
            #ifdef DEBUG
               printf("Got a connection packet\n");
            #endif
            if ((pid = fork()) < 0) {
               perror("fork");
               exit(-1);
            }
            // Child process
            if (pid == 0) {
               process_client(server_sk_num, buf, recv_len, &client);
               exit(0);
            }
         }

         while (waitpid(-1, &status, WNOHANG) > 0) {}
      }
   }
}

void process_client(int32_t server_sk_num, uint8_t *buf, int32_t recv_len, Connection *client)
{
   STATE state = START;
   DataPacket *winBuffer = NULL;
   int32_t data_file = 0;
   int32_t buf_len = recv_len;
   int32_t packet_len = 0;
   uint8_t packet[MAX_LEN];
   uint32_t win_size = 0;
   uint32_t buf_size = 0;
   uint32_t seq_num = START_SEQ_NUM;
   uint32_t lowFrame = seq_num;

   client->sk_num = 0;
   // Here's a nice state machine! :)
   #ifdef DEBUG
      printf("In process_client\n");
   #endif
   while (state != DONE) {
      switch (state)
      {
         case START:
            state = SETUP_ACK;
            break;

         case SETUP_ACK:
            /* Packet is temporarily treated as a new kind of buffer */
            state = setup_ack(client, buf, &buf_len, &win_size, &buf_size, seq_num);
            break;

         case FILENAME:
            #ifdef DEBUG
               printf("STATE - Filename\n");
            #endif
            state = filename(client, buf, buf_len, &data_file, &winBuffer, win_size);
            break;

         case SEND_DATA:
            #ifdef DEBUG
               printf("STATE - Sending Data...\n");
            #endif
            state = send_data(client, packet, &packet_len, data_file, winBuffer, win_size, buf_size, &seq_num, lowFrame);
            break;

         case QUICK_WAIT:
            #ifdef DEBUG
               printf("STATE - Quick Wait...\n");
            #endif
            state = quick_wait(client, winBuffer, win_size, &lowFrame, &seq_num);
            break; 

         case PROCESS_INCOMING:
            #ifdef DEBUG
               printf("STATE - Process Incoming...\n");
            #endif
            state = process_incoming(client, winBuffer, win_size, &lowFrame, &seq_num);
            break;

         case WAIT_ON_RR:
            #ifdef DEBUG
               printf("STATE - Wait on RR...\n");
            #endif
            state = wait_on_rr(client, winBuffer, win_size, &lowFrame, &seq_num, packet, &packet_len);
            break;

         case TIMEOUT_ON_RR:
            #ifdef DEBUG
               printf("STATE - Timeout on RR...\n");
            #endif
            state = timeout_on_rr(client, packet, packet_len);
            break;

         case WAIT_ON_EOF_ACK:
            state = wait_on_eof_ack(client);
            break;

         case TIMEOUT_ON_EOF_ACK:
            state = timeout_on_eof_ack(client, packet, packet_len);
            break;

         case DONE:
            #ifdef DEBUG
            printf("Lowest Frame (Raw/Hashed): %u %u\n", lowFrame, lowFrame % win_size);
            #endif
            break;

         default:
            printf("I'm in default...this shouldn't be happening...\n");
            state = DONE;
            break;
      }
   }
}

STATE setup_ack(Connection *client, uint8_t *buf, int32_t *buf_len, uint32_t *win_size,
   uint32_t *buf_size, uint32_t seq_num)
{
   STATE ns = SETUP_ACK;
   uint8_t flag = 0;
   uint32_t num = 0;
   uint8_t pkt[MAX_LEN];

   /* Create client socket to allow for processing this particular client */
   if (client->sk_num == 0 && (client->sk_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("setup_ack, open client socket");
      exit(-1);
   }

   memcpy(win_size, buf, sizeof(uint32_t));
   memcpy(buf_size, buf + sizeof(uint32_t), sizeof(uint32_t));
   *win_size = ntohl(*win_size);
   *buf_size = ntohl(*buf_size);

   send_buf(NULL, 0, client, ACK_SETUP, seq_num, pkt);
   if (select_call(client->sk_num, LONG_TIME, 0, NOT_NULL) == 0) {
      printf("Timeout after 10 seconds, rcopy client is probably gone\n");
      return DONE;
   }

   *buf_len = recv_buf(buf, MAX_LEN, client->sk_num, client, &flag, &num);   
   if (*buf_len != CRC_ERROR) {
      if (flag == FNAME) {
         ns = FILENAME;
      }
      else if (flag == SETUP) {
         ns = SETUP_ACK;
      }
   }
   return ns;
}

STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file,
   DataPacket **window, uint32_t win_size)
{
   uint8_t response[1];
   char fname[MAX_LEN];
   STATE returnValue = DONE;

   // Extract filename
   memcpy(fname, buf, recv_len);

   if (( (*data_file) = open(fname, O_RDONLY) ) < 0) {
      #ifdef DEBUG
         printf("File problem...\n");
         printPkt(buf, recv_len);
      #endif
      send_buf(response, 0, client, FNAME_BAD, 0, buf);
      returnValue = DONE;
   }
   else {
      #ifdef DEBUG
         printf("File's okay...\n");
      #endif
      send_buf(response, 0, client, FNAME_OK, 0, buf);
      if ((*window = calloc(win_size, sizeof(DataPacket))) == NULL) {
         perror("filename, malloc");
         exit(-1);
      }
      returnValue = SEND_DATA;
   }
   return returnValue;
}

STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file,
   DataPacket *window, int32_t win_size, int32_t buf_size, uint32_t *seq_num, uint32_t lowFrame)
{
   uint8_t buf[MAX_LEN];
   int32_t len_read = 0;
   int index = *seq_num % win_size;
   STATE returnValue = DONE;

   #ifdef DEBUG
      printf("***Buffer Size: %d\n", buf_size);
      printf("***File descriptor: %d\n", data_file);
      printf("***Window Ptr: %p\n", window);
      printf("***lowFrame (Raw/Hashed): %u %u\n", lowFrame, lowFrame % win_size);
      printf("***Seq Num (Raw/Hashed): %u %u\n", *seq_num, *seq_num % win_size);
   #endif

   // Where do we take our packet from?
   //   1. We reached our window limit...
   //   2. We did not reach our window limit, but we might have to resend packets
   //   3. We need to read new data from the file.
   // These may or may not cover our EOF case.
   if (*seq_num > (lowFrame + win_size - 1) && window[index].inWindow) {
      // I don't know that I actually need to check the window[index].inWindow variable 
      //    if the *seq_num check exists.
      return WAIT_ON_RR;
   } 
   else if (*seq_num <= (lowFrame + win_size - 1) && window[index].inWindow) {
      len_read = window[index].inWindow;
      packet = window[index].packet;
   }
   else {
      #ifdef DEBUG
         printf("***Reading from file to send data...\n");
      #endif
      len_read = read(data_file, buf, buf_size);
   }

   // Is this really the end condition we want for the send_data portion?
   // lowFrame == *seq_num means we must have received the RR for the last data packet
   //    created from the file.
   if (len_read == 0 && lowFrame == *seq_num && window[lowFrame % win_size].inWindow == 0) {
      (*packet_len) = send_buf(buf, 1, client, END_OF_FILE, *seq_num, packet);
      return WAIT_ON_EOF_ACK;
   }
   else if (len_read == 0) {
      // Go and wait for the last RR to come in.
      return WAIT_ON_RR;
   }

   switch(len_read)
   {
      case -1:
         perror("send_data, read error");
         returnValue = DONE;
         break;

      case 0:
         (*packet_len) = send_buf(buf, 1, client, END_OF_FILE, *seq_num, packet);
         returnValue = WAIT_ON_EOF_ACK;
         break;

      default:
         #ifdef DEBUG
            printf("Server Sending Seq. Num. %u\n", *seq_num);
         #endif

         if (window[index].inWindow == 0) {
            (*packet_len) = send_buf(buf, len_read, client, DATA, *seq_num, packet);

            /* Add the packet to the window buffer,
                  an 'inWindow' of greater than 0 implies a packet-in-use */
            memcpy(window[index].packet, packet, *packet_len);
            window[index].inWindow = *packet_len;
         }
         else {
            (*packet_len) = safeSend(packet, len_read, client);
         }
         #ifdef DEBUG_PKT
            printf("Packet Len: %u\n", window[index].inWindow);
            printPkt(packet, window[index].inWindow);
         #endif
         (*seq_num)++;
         returnValue = QUICK_WAIT;
         break;
   }
   return returnValue;
}

STATE timeout_on_rr(Connection *client, uint8_t *packet, int32_t packet_len)
{
   #ifdef DEBUG_PKT
      printf("Packet Length: %u\n", packet_len);
      printf("Client->len: %d\n", client->len);
   #endif
   if (sendtoErr(client->sk_num, packet, packet_len, 0,
      (struct sockaddr *) &(client->remote), client->len) < 0)
   {
      perror("timeout_on_rr sendto");
      exit(-1);
   }
   return WAIT_ON_RR;
}

STATE timeout_on_eof_ack(Connection *client, uint8_t *packet, int32_t packet_len)
{
   if (sendtoErr(client->sk_num, packet, packet_len, 0,
      (struct sockaddr *) &(client->remote), client->len) < 0)
   {
      perror("timeout_on_eof_ack sendto");
      exit(-1);
   }
   return WAIT_ON_EOF_ACK;
}

STATE quick_wait(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num)
{
   STATE ns = SEND_DATA;
   uint32_t crc_check = 0;
   uint8_t buf[MAX_LEN];
   int32_t len = MAX_LEN;
   uint8_t flag = 0;
   uint32_t temp_seq_num = 0;
   int index;

   /* Have a '0-second' wait time to check for any packets on the line */
   if (select_call(client->sk_num, 0, 0, NOT_NULL)) {
      crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &temp_seq_num); 
   }
   else {
      // If there was no packet...
      return ns;
   }
   
   // If there was a packet...
   memcpy(&temp_seq_num, buf, sizeof(uint32_t));
   temp_seq_num = ntohl(temp_seq_num);   // Move seq_num's back to the rejected packet
   if (crc_check == CRC_ERROR) {
      #ifdef DEBUG
         printf("---   CRC_ERROR   ---\n");
      #endif
      return QUICK_WAIT;   // Ignore any CRC errors and try to send packets.
   }
   else if (flag == REJ) {
      #ifdef DEBUG
         printf("***Packet to be resent: %d\n", temp_seq_num);
      #endif
      *seq_num = temp_seq_num;
      ns = SEND_DATA;
   }
   else if (flag == RR) {
      ns = QUICK_WAIT;   // This had no effect on the outcome of repeated packets... :/
      //ns = SEND_DATA;
   }
   else {
      printf("There's a weird flag in our midst! FLAG: %d\n", flag);
      return DONE;
   }
   #ifdef DEBUG
       printf("***CLEARING DATA UP TO TEMP_SEQ_NUM in quick_wait\n");
   #endif
   // Eliminate all packets in the window that are good (set inWindow to 0).
   for (index = *lowFrame; index < temp_seq_num; index++) {
      window[index % win_size].inWindow = 0;
   }
   if (*lowFrame < temp_seq_num) 
      *lowFrame = temp_seq_num;
   /* If RR's come in that push our window up and we were in the process of resending
         a window due to a REJ */
   if (*seq_num < temp_seq_num)
      *seq_num = temp_seq_num;
   return ns;
}

/* Gets any RR or REJ packets that are coming in on the line.
 * ----------------------------- DEPRECATED FUNCTION -------------------------------
 */
STATE process_incoming(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num)
{
   STATE ns = SEND_DATA;
   uint32_t crc_check = 0;
   uint8_t buf[MAX_LEN];
   int32_t len = MAX_LEN;
   uint8_t flag = 0;
   uint32_t temp_seq_num = 0;
   int index;

   /* Have a '0-second' wait time to check for any packets on the line */
   if (select_call(client->sk_num, 0, 0, NOT_NULL)) {
      crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &temp_seq_num); 
   }
   else {
      // If there was no packet...return to sending data
      return ns;
   }

   if (crc_check == CRC_ERROR) {
      // Perhaps there are better packets down the line...
      ns  = PROCESS_INCOMING;
   }
   else if (flag == RR || flag == REJ) {
      // If there was a packet...
      memcpy(&temp_seq_num, buf, sizeof(uint32_t));
      temp_seq_num = ntohl(temp_seq_num);   // Move seq_num's back to the rejected packet
      if (flag == RR)
         ns = PROCESS_INCOMING;
      // Eliminate all packets in the window that are good (set inWindow to 0).
      for (index = *lowFrame; index < temp_seq_num; index++) {
         window[index % win_size].inWindow = 0;
      }
      if (*lowFrame < temp_seq_num) 
         *lowFrame = temp_seq_num;
      if (flag == REJ)
         *seq_num = temp_seq_num;
   }
   else {
      printf("There's a weird flag in our midst! FLAG: %d\n", flag);
      return DONE;
   }
   return ns;
}
/* ----------------------------- DEPRECATED FUNCTION ABOVE -------------------------- */

/* wait_on_rr only occurs if send_data reaches the limit of the window, set down in setup_ack.
 *    The only way out is through receiving an actual RR packet.
 */
STATE wait_on_rr(Connection *client, DataPacket *window, uint32_t win_size, uint32_t *lowFrame,
   uint32_t *seq_num, uint8_t *pkt, int32_t *pkt_len)
{
   STATE returnValue = DONE;
   uint32_t crc_check = 0;
   uint8_t buf[MAX_LEN];
   int32_t len = MAX_LEN;
   uint8_t flag = 0;
   uint32_t temp_seq_num = 0;
   int i;
   static int retryCount = 0;

   returnValue = processSelect(client, &retryCount, TIMEOUT_ON_RR, SEND_DATA, DONE);
   if (returnValue == SEND_DATA) {
      crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &temp_seq_num);

      memcpy(&temp_seq_num, buf, sizeof(uint32_t));
      temp_seq_num = ntohl(temp_seq_num);
      // If CRC error ignore packet
      if (crc_check == CRC_ERROR) {
         #ifdef DEBUG
            printf("---   CRC_ERROR   ---\n");
         #endif
         return WAIT_ON_RR;
      }
      else if (flag == RR) {
         returnValue = QUICK_WAIT;
      }
      else if (flag == REJ) {
         //returnValue = SEND_DATA;
         *seq_num = temp_seq_num;
      }
      else if (flag != RR && flag != REJ) {
         #ifdef DEBUG
            printf("In wait_on_rr but not an RR or REJ flag; it is: %d\n", flag);
         #endif
         return DONE;
      }
      #ifdef DEBUG
         printf("***'CLEARING' UP DATA TO TEMP_SEQ_NUM in wait_on_rr\n");
      #endif
      // Move our lowest frame up...
      for (i = *lowFrame; i < temp_seq_num; i++) {
         window[i % win_size].inWindow = 0;
      }
      if (*lowFrame < temp_seq_num)
         *lowFrame = temp_seq_num;
      //if (*seq_num < temp_seq_num)
      *seq_num = temp_seq_num;
   }

   // Prepare the lowest frame packet to be sent again.
   if (returnValue == TIMEOUT_ON_RR) {
      *pkt_len = window[(*lowFrame) % win_size].inWindow;
      memcpy(pkt, window[(*lowFrame) % win_size].packet, *pkt_len);
      #ifdef DEBUG
         printf("Window's Packet Length: %d\n", window[(*lowFrame) % win_size].inWindow);
         printf("Low Frame to Send: %u\n", *lowFrame % win_size);
         printf("Supposed Packet Len: %d\n", *pkt_len);
         printPkt(window[(*lowFrame) % win_size].packet, *pkt_len);
      #endif
   }
   return returnValue;
}

STATE wait_on_eof_ack(Connection *client)
{
   STATE returnValue = DONE;
   uint32_t crc_check = 0;
   uint8_t buf[MAX_LEN];
   int32_t len = MAX_LEN;
   uint8_t flag = 0;
   uint32_t seq_num = 0;
   static int retryCount = 0;

   returnValue = processSelect(client, &retryCount, TIMEOUT_ON_EOF_ACK, DONE, DONE);
   if (returnValue == DONE) {
      crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &seq_num);

      // If CRC error ignore packet
      if (crc_check == CRC_ERROR) {
         #ifdef DEBUG
            printf("---   CRC_ERROR   ---\n");
         #endif
         returnValue = WAIT_ON_EOF_ACK;
      }
      else if (flag != EOF_ACK) {
         #ifdef DEBUG
            printf("In wait_on_eof_ack but not an EOF_ACK flag; it is: %d...Oh well, probably a leftover RR\n", flag);
         #endif
         returnValue = WAIT_ON_EOF_ACK;
      }
      else {
         #ifdef DEBUG
            printf("File transfer completed successfully!\n");
         #endif
         returnValue = DONE;
      }
   }
   return returnValue;
}

int processArgs(int argc, char **argv)
{
   int portNumber = 0;

   if (argc < 2 || argc > 3) {
      printf("Usage: %s error_rate [port number]\n", argv[0]);
      exit(-1);
   }

   if (argc == 3) {
      portNumber = atoi(argv[2]);
   }
   else {
      portNumber = 0;
   }

   return portNumber;
}
