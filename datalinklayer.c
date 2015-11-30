#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include "linkfunctions.h"
#include "physicallayer.h"

char inboundQUEUE [MAX_QUEUE][FRAME_SIZE];
int inboundQMarker = 0;
int inboundFrameCurrent = 0;

char upboundQUEUE [MAX_QUEUE][MESSAGE_SIZE]; // Holds MESSAGES that have been acked but the APP layer doesn't want yet.
int upQCurrent = 0; // Marks the next open space to enqueue a message
int upQSend = 0; // How many packets are in upboundQ

char outboundQUEUE [MAX_QUEUE][FRAME_SIZE];
int outboundNUMS [MAX_QUEUE];
int connected = 0;
int outQueueCount = 0;
int outQueueCurrent = 0;

int outboundFrameCurrent = 1;

int transmissionMode = GOBACKN;

int statDataSent = 0;
int statReSent = 0;
int statAckSent = 0;
int statAckRcvd = 0;
int statDupRcvd = 0;
int statDataVolume = 0;
int statTimeToClassifyAvg = 0;

int launchPacket(char* packet, int* frame, int size)
{
  printf("Send frame to physical\n");
  physicalSend(packet, size);
  pthread_t timer_thread;
  
  if(pthread_create(&timer_thread, NULL, (void *)send_timer, frame))
    { fprintf(stderr, "Error creating timer thread\n"); return 1;}
  return 0;
}

int statDump()
{
  printf("Number of data packets Sent: %d\n", statDataSent);
  printf("Number of retransmissions: %d\n", statReSent);
  printf("Number of ACKs sent: %d\n", statAckSent);
  printf("Number of ACKs received: %d\n", statAckRcvd);
  printf("Number of dupliate packets: %d\n", statDupRcvd);
  printf("Volume of data sent in packets: %d\n", statDataVolume);
  return 0;
}

int dataLinkRecv(char* buffer)
{
  int i;
  //inQueueCurrent = (inQueueCurrent + 1)%MAX_QUEUE;
  while(upQSend == 0)
  {
    // Wait for the receiving thread to actually come up with some data.

    // Look around the receive buffer for the next packet.
    if(transmissionMode == SELECTIVE_REPEAT)
    {
      for(i = 0; i < MAX_QUEUE; i++)
      {
	if(atoi(inboundQUEUE[i]+IDX_NUM) == inboundFrameCurrent+1)
	{
	  strcpy(upboundQUEUE[upQCurrent], inboundQUEUE[i]+IDX_MESSAGE);
	  inboundFrameCurrent += 1;
	  upQCurrent = (upQCurrent + 1)%MAX_QUEUE;
	  upQSend ++;
	  break;
	}
      }
    }
  }
  // Copy data from the queue into the buffer.
  strcpy(buffer, upboundQUEUE[upQSend]);
  //  Mark that this message has been delivered
  upQSend--;
  return 0;
}

int fromPhysHandleAck(char* buffer, int frameNumRcvd)
{
  statAckRcvd++;
  
  int idx_q = -1;
  int i;
  for(i = 0; i < MAX_QUEUE; i++)
  {
    if(outboundNUMS[i] == frameNumRcvd)
    { idx_q = i; }
    if(outboundNUMS[i] <= frameNumRcvd)
    { outboundNUMS[i] = 0; }
  }
  // Received ACK for a packet we never sent
  if(idx_q == -1)
  {
    printf("ERROR.  Received ACK for unknown packet.\n");
  }
  outQueueCount--;
  // stop retransmission timer. handled in send_timer by outboundNUMS having been set
  if(outQueueCount == 0){ }
  // Reset retransmission timer. handled in send_timer by there being a separate timer
  else { }
  return 0;
}

int fromPhysRecv(char* buffer)
{
  int type = (int)buffer[IDX_TYPE] - '0';
  int frameNumRcvd = atoi(buffer+IDX_NUM);
  int sizeRcvd = atoi(buffer+IDX_SIZE);

  // If the application isn't reading, don't keep reading.
  while(upQSend == upQCurrent -1)
  {
    //do nothing
  }
     
  //Check if it's an ACK
  if(type == TYPE_ACK)
  {
    printf("message is an ACK\n");
    fromPhysHandleAck(buffer, frameNumRcvd);
  }
  else if(type == TYPE_MESSAGE)
  {
    printf("message is a message\n");
    char buffer_ack[FRAME_SIZE];
    if(checksumCheck(buffer, IDX_CRC-1) != 0)
    {
      perror("Checksum doesn't match, packet corrupt.\n");
      reACKnowledge(buffer_ack);
      return ERR_CORRUPT;
    }
    else
      printf("Checksums match.\n");
    
    // in-order packet
    if(frameNumRcvd == inboundFrameCurrent+1)
    {
      inboundFrameCurrent = frameNumRcvd;
      // ACK!
      constructAck(buffer, inboundFrameCurrent);
      physicalSend(buffer, IDX_END);

      // Store the packet for the app-layer to retrieve
      strcpy(upboundQUEUE[upQCurrent], buffer+IDX_MESSAGE);
      upQCurrent = (upQCurrent + 1)%MAX_QUEUE;
      upQSend ++;
    }
    // Ack any appropriate packets individually.
    if(transmissionMode == SELECTIVE_REPEAT)
    {
      // Ack that packet, whatever it was.
      constructAck(buffer, frameNumRcvd);
      physicalSend(buffer, IDX_END);
      // Put it in the queue, getting them out in order is DataLinkRecv's problem
      strcpy(inboundQUEUE[inboundQMarker], buffer);
      inboundQMarker = (inboundQMarker + 1) % MAX_QUEUE;
    }
    else //GBN
    {
      // Duplicate packet received.
      if(frameNumRcvd < inboundFrameCurrent)
      {
	statDupRcvd++;
	reACKnowledge(buffer_ack);
      }
      // Out of window packet received somehow
      else if(frameNumRcvd > inboundFrameCurrent + MAX_QUEUE)
      {
	printf("WARNING: Received spurious ACK.\n");
	reACKnowledge(buffer_ack);
      }
      else
      {
	reACKnowledge(buffer_ack);
      }
    }
  }
  else
  {
    printf("ERROR.  Unknown frame  type.\n");
  }
  
  return 0;
}


/*============================================================================*/
// Ack Stuff
/*============================================================================*/
int constructAck(char* buffer, int frame) //in this buffer
{
  statAckSent++;
  
  memset(buffer, 0, IDX_END);
  sprintf(buffer+IDX_SIZE, "%d", 0); 
  //buffer[IDX_SIZE] = '0';
  buffer[IDX_TYPE] = TYPE_ACK + '0';
  sprintf(buffer+IDX_NUM, "%d", frame);
    //strcpy(buffer+IDX_NUM, itoa(frame));
  unsigned crc = crc8(0, buffer, IDX_CRC-1);
  //printf("Adding CRC of %u\n", crc);
  sprintf(buffer+IDX_CRC, "%d", crc);
  //buffer[IDX_CRC] = crc + '0';
  return 0;
}

int reACKnowledge(char* buffer)
{
  constructAck(buffer, outboundFrameCurrent);
  physicalSend(buffer, IDX_END);
  return 0;
}

/*============================================================================*/
// Sending messages
/*============================================================================*/
int send_timer(int* target)
{
  clock_t start = clock(), diff;
  int msec;
  int frame = *target;
  int present = 0;
  int i;
  while(1)
  {
    diff = clock() - start;
    msec = diff * 1000 / CLOCKS_PER_SEC;
    //check if it's done.
    //printf("MSEC: %d, FRAME: %d\n", msec, frame);
    present = 0;
    for(i = 0; i < MAX_QUEUE; i++)
    {
      //printf("%d\n", outboundNUMS[i]);
      if(outboundNUMS[i] == frame)
      { present = i+1; }
    }
    if(!present)
    {
      printf("frame no longer OUTBOUND.\n");
      return 0;
    }
    if(msec > 1000)
    {
      printf("WARNING: No ack within timeout window for frame %d\n", frame);

      if(transmissionMode == GOBACKN)
      {
      // Timer expired, resend everything
      for(i = 0; i < MAX_QUEUE; i++)
      {
	if(outboundNUMS[i] != 0)
	{
	  statDataSent++;
	  statReSent++;
	  int sizeRcvd = atoi(outboundQUEUE[i]+IDX_SIZE);
	  statDataVolume += sizeRcvd;
	  
	  launchPacket(outboundQUEUE[i], &frame, sizeRcvd);
	}
      }
      }
      else
      {
	int sizeRcvd = atoi(outboundQUEUE[present-1]+IDX_SIZE);
	launchPacket(outboundQUEUE[present-1], &frame, sizeRcvd);
      }
      return 1;
    }
  }
}

//GBN
//If too many in flight, buffer.
//if buffer also full, error. Otherwise, send.
//ACK cumulatively clears all lower numbers
//Timeout resends all outstanding packets (the whole Q)

//SR
//Buffer packets until the right one comes in.
//Ack all incoming packets.
int dataLinkSend(char *buffer, int n)
{
  int target;
  //if(!connected)
  //connectToServer();
  //printf("GOT FROM APP: %s\n", buffer);
  
  if(outQueueCount < MAX_QUEUE)
  {
    statDataSent ++;
    statDataVolume += n;
    
    // Write the message into the Outgoing Q
    memset(outboundQUEUE[outQueueCount], 0, IDX_END);
    sprintf(outboundQUEUE[outQueueCount]+IDX_SIZE, "%d", n);
    //outboundQUEUE[outQueueCount][IDX_SIZE] = '1';
    outboundQUEUE[outQueueCount][IDX_TYPE] = TYPE_MESSAGE + '0';
    strncpy(outboundQUEUE[outQueueCount]+IDX_MESSAGE, buffer, n);
    sprintf(outboundQUEUE[outQueueCount]+IDX_NUM, "%d", outboundFrameCurrent);
      //strcpy(outboundQUEUE[outQueueCount]+IDX_NUM, itoa(outboundFrameCurrent));
    unsigned crc = crc8(0, outboundQUEUE[outQueueCount], IDX_CRC-1);
    //printf("Adding CRC of %u\n", crc);
    sprintf(outboundQUEUE[outQueueCount]+IDX_CRC, "%d", crc);
    //outboundQUEUE[outQueueCount][IDX_CRC] = crc + '0';
    
    outboundNUMS[outQueueCurrent] = outboundFrameCurrent;
    target = outboundFrameCurrent;

    printf("%s %s %s %s %s\n",
	   outboundQUEUE[outQueueCurrent]+IDX_SIZE,
	   outboundQUEUE[outQueueCurrent]+IDX_NUM,
	   outboundQUEUE[outQueueCurrent]+IDX_TYPE,
	   outboundQUEUE[outQueueCurrent]+IDX_MESSAGE,
	   outboundQUEUE[outQueueCurrent]+IDX_CRC);

    /*printf("SIZE   : %s\n", outboundQUEUE[outQueueCurrent]+IDX_SIZE);
    printf("NUM    : %s\n", outboundQUEUE[outQueueCurrent]+IDX_NUM);
    printf("TYPE   : %s\n", outboundQUEUE[outQueueCurrent]+IDX_TYPE);
    printf("MESG   : %s\n", outboundQUEUE[outQueueCurrent]+IDX_MESSAGE);
    printf("CRC    : %s\n", outboundQUEUE[outQueueCurrent]+IDX_CRC);
    printf("\n");*/
    launchPacket(outboundQUEUE[outQueueCurrent], &target, n);
    
    /*physicalSend(outboundQUEUE[outQueueCurrent]);
    pthread_t timer_thread;
  
    if(pthread_create(&timer_thread, NULL, (void *)send_timer, &target))
    { fprintf(stderr, "Error creating timer thread\n"); return 1;}*/

    // Update the state of the in-flight messages.
    outQueueCount++;
    outQueueCurrent = (outQueueCurrent + 1)%MAX_QUEUE;
    outboundFrameCurrent++;
  }
  else
  {
    perror("Too many packets already in flight.  Window full.\n");
    return 1;
  }
  return 0;
}
/*============================================================================*/
// Checksum
/*============================================================================*/
int checksumCheck(char* buffer, int size)
{
  unsigned crcRcvd = atoi(buffer + IDX_CRC);
  unsigned crcSent = crc8(0, buffer, size);
  printf("Comparing %u to %u\n", crcRcvd, crcSent);
  return crcRcvd != crcSent;
}

static unsigned char crc8_table[] =
{
    0x00, 0x3e, 0x7c, 0x42, 0xf8, 0xc6, 0x84, 0xba, 0x95, 0xab, 0xe9, 0xd7,
    0x6d, 0x53, 0x11, 0x2f, 0x4f, 0x71, 0x33, 0x0d, 0xb7, 0x89, 0xcb, 0xf5,
    0xda, 0xe4, 0xa6, 0x98, 0x22, 0x1c, 0x5e, 0x60, 0x9e, 0xa0, 0xe2, 0xdc,
    0x66, 0x58, 0x1a, 0x24, 0x0b, 0x35, 0x77, 0x49, 0xf3, 0xcd, 0x8f, 0xb1,
    0xd1, 0xef, 0xad, 0x93, 0x29, 0x17, 0x55, 0x6b, 0x44, 0x7a, 0x38, 0x06,
    0xbc, 0x82, 0xc0, 0xfe, 0x59, 0x67, 0x25, 0x1b, 0xa1, 0x9f, 0xdd, 0xe3,
    0xcc, 0xf2, 0xb0, 0x8e, 0x34, 0x0a, 0x48, 0x76, 0x16, 0x28, 0x6a, 0x54,
    0xee, 0xd0, 0x92, 0xac, 0x83, 0xbd, 0xff, 0xc1, 0x7b, 0x45, 0x07, 0x39,
    0xc7, 0xf9, 0xbb, 0x85, 0x3f, 0x01, 0x43, 0x7d, 0x52, 0x6c, 0x2e, 0x10,
    0xaa, 0x94, 0xd6, 0xe8, 0x88, 0xb6, 0xf4, 0xca, 0x70, 0x4e, 0x0c, 0x32,
    0x1d, 0x23, 0x61, 0x5f, 0xe5, 0xdb, 0x99, 0xa7, 0xb2, 0x8c, 0xce, 0xf0,
    0x4a, 0x74, 0x36, 0x08, 0x27, 0x19, 0x5b, 0x65, 0xdf, 0xe1, 0xa3, 0x9d,
    0xfd, 0xc3, 0x81, 0xbf, 0x05, 0x3b, 0x79, 0x47, 0x68, 0x56, 0x14, 0x2a,
    0x90, 0xae, 0xec, 0xd2, 0x2c, 0x12, 0x50, 0x6e, 0xd4, 0xea, 0xa8, 0x96,
    0xb9, 0x87, 0xc5, 0xfb, 0x41, 0x7f, 0x3d, 0x03, 0x63, 0x5d, 0x1f, 0x21,
    0x9b, 0xa5, 0xe7, 0xd9, 0xf6, 0xc8, 0x8a, 0xb4, 0x0e, 0x30, 0x72, 0x4c,
    0xeb, 0xd5, 0x97, 0xa9, 0x13, 0x2d, 0x6f, 0x51, 0x7e, 0x40, 0x02, 0x3c,
    0x86, 0xb8, 0xfa, 0xc4, 0xa4, 0x9a, 0xd8, 0xe6, 0x5c, 0x62, 0x20, 0x1e,
    0x31, 0x0f, 0x4d, 0x73, 0xc9, 0xf7, 0xb5, 0x8b, 0x75, 0x4b, 0x09, 0x37,
    0x8d, 0xb3, 0xf1, 0xcf, 0xe0, 0xde, 0x9c, 0xa2, 0x18, 0x26, 0x64, 0x5a,
    0x3a, 0x04, 0x46, 0x78, 0xc2, 0xfc, 0xbe, 0x80, 0xaf, 0x91, 0xd3, 0xed,
    0x57, 0x69, 0x2b, 0x15
};


unsigned crc8(unsigned crc, char* buffer, size_t len)
{
    char* end;

    if (len == 0)
      return crc;
    crc ^= 0xff;
    end = buffer + len;
    do
    {
      crc = crc8_table[crc ^ *buffer++];
    } while (buffer < end);
    return crc ^ 0xff;
}


/*int connectToServer()
{
  // physical layer setup socket between server and client.
  physicalConnect();
  connected = 1;

  pthread_t reader_thread;
  
  if(pthread_create(&reader_thread, NULL, read_frames, NULL))
  {
    fprintf(stderr, "Error creating reader thread\n");
    exit(1);
    }
  return 0;
}*/

/*
//receiving thread
// while the upbound buffer isn't full, call fromPhysRecv
void *read_frames()
{
  int n;
  char chat_buffer[FRAME_SIZE];
  while(1)
  {
    // If the application isn't reading, don't keep reading.
    if(upQSend == upQCurrent -1)
      continue;
    // read server response
    memset(chat_buffer, 0, FRAME_SIZE);
    physicalRecv(chat_buffer);
    n = fromPhysRecv(chat_buffer);
    if(n < 0)
    { sleep(1); } //sleep some time while waiting for a message

    else
    {
      //TODO maybe something
    }
  }		
  }*/

// 8-bit CRC with polynomial x^8+x^6+x^3+x^2+1, 0x14D.
