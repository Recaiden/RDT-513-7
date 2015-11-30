#ifndef PHYS_H_
#define PHYS_H

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_CLIENTS 1
#define MAX_SOCKETS  MAX_CLIENTS+5
#define PORT 6660

#define SERVER 1
#define CLIENT 2

int initServer();
int initClient();
int handleMessage(char *buffer);
void * waitForResponse(void *socket);
void shuffle(int *array, size_t n);
void physicalSend(char *buffer);
void setRates(int drop, int corr);

#endif
