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

int launchPacket(char* packet, int* frame)
{
  pthread_t timer_thread;
  
  if(pthread_create(&timer_thread, NULL, (void *)send_timer, frame))
    { fprintf(stderr, "Error creating timer thread\n"); return 1;}
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
    if(checksumCheck(buffer, sizeRcvd) != 0)
      {
	reACKnowledge(buffer_ack);
	perror("Checksum doesn't match, packet corrupt.\n");
	return ERR_CORRUPT;
      }
    
    // in-order packet
    if(frameNumRcvd == inboundFrameCurrent+1)
    {
      inboundFrameCurrent = frameNumRcvd;
      // ACK!
      constructAck(buffer, inboundFrameCurrent);
      physicalSend(buffer);

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
      physicalSend(buffer);
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
    printf("ERROR.  Unknown frame  type.");
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
  unsigned crc = crc8(0, buffer, FRAME_SIZE-1);
  printf("Adding CRC of %u\n", crc);
  buffer[IDX_CRC] = crc + '0';
  return 0;
}

int reACKnowledge(char* buffer)
{
  constructAck(buffer, outboundFrameCurrent);
  physicalSend(buffer);
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
      { present = 1; }
    }
    if(!present)
    {
      printf("frame no longer OUTBOUND.\n");
      return 0;
    }
    if(msec > 1000)
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
	  
	  //physicalSend(outboundQUEUE[i]);
	  launchPacket(outboundQUEUE[i], &frame);
	}
      }

      printf("WARNING: No ack within timeout window for frame %d\n", frame);
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
    unsigned crc = crc8(0, outboundQUEUE[outQueueCount], n+32+32+1);
    printf("Adding CRC of %u\n", crc);
    outboundQUEUE[outQueueCount][IDX_CRC] = crc + '0';
    
    outboundNUMS[outQueueCurrent] = outboundFrameCurrent;
    target = outboundFrameCurrent;


    printf("Send frame to physical\n");
    launchPacket(outboundQUEUE[outQueueCurrent], &target);
    
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
  return crcRcvd == crcSent;
}

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
