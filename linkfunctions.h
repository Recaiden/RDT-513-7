#ifndef LINK_H
#define LINK_H
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#define MESSAGE_SIZE 100

#define IDX_SIZE 0
#define IDX_NUM IDX_SIZE+32*1
#define IDX_TYPE  IDX_NUM+32*1
#define IDX_MESSAGE IDX_TYPE+1*1
#define IDX_CRC IDX_MESSAGE+MESSAGE_SIZE*1
#define IDX_END IDX_CRC+8*1

#define FRAME_SIZE IDX_END

#define MAX_QUEUE 5

#define ERR_CORRUPT 1

#define TYPE_ACK 2
#define TYPE_MESSAGE 1

#define GOBACKN 1
#define SELECTIVE_REPEAT 2

int statDump();
int dataLinkRecv(char* buffer);
int fromPhysHandleAck(char* buffer, int frameNumRcvd);
int fromPhysRecv(char* buffer);
int constructAck(char* buffer, int frame);
int reACKnowledge(char* buffer);
int send_timer(int* target);
int dataLinkSend(char *buffer, int size);
int checksumCheck(char* buffer, int size);
unsigned crc8(unsigned crc, char* buffer, size_t len);

#endif
