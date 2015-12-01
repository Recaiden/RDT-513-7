
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "linkfunctions.h"
#include "physicallayer.h"

int init = 0;
int client_id = 0;

//these rates will be an integer out of 100 will get these from globals in appclient and server
int dropRate = 0;
int corruptRate = 0;
int communicator = 0;
int server_id = 0;

void setRates(int drop, int corr){
  dropRate = drop;
  corruptRate = corr;
}

/**
handleMessage: sends recieved buffer to the link layer
*/
int handleMessage(char *buffer){
  fromPhysRecv(buffer);
  return 0;
}

/**
waitForResponse: threaded function which waits to read data from the server or client.
The data is then passed to handle message to be sent to the link layer.
*/
void * waitForResponse(void *socket)
{
  printf("Waiting for response.\n");
  int n;
  int * socket_fd = (int *)socket;
  char chat_buffer[FRAME_SIZE];
  while(1)
  {
    //read server response
    //bzero(chat_buffer, FRAME_SIZE);
    memset(chat_buffer, 0, FRAME_SIZE);

    n = read((* socket_fd), chat_buffer, FRAME_SIZE);
    printf("RECEIVED: %d %s\n", *socket_fd, chat_buffer);
    if(n < 0)
    {
      sleep(1); //sleep some time while waiting for a message
    }
    else
    {
      printf("n: %d, socket message Recieved\n", n);
      handleMessage(chat_buffer);
    }
  }
}

void * waitForResponseServer(void *socket)
{
  printf("Waiting for response.\n");
  int n;
  //int * socket_fd = (int *)socket;
  int socket_fd = *(int*)socket;
  char buffer[FRAME_SIZE];
  while(1)
  {
    //read server response
    //bzero(chat_buffer, FRAME_SIZE);
    memset(buffer, 0, FRAME_SIZE);

    n = read((socket_fd), buffer, FRAME_SIZE);
    printf("RECEIVED from %d \n", socket_fd);
    if(n < 0)
    {
      sleep(1); //sleep some time while waiting for a message
    }
    else
    {
      printf("SIZE   : %s\n", buffer+IDX_SIZE);
      printf("NUM    : %s\n", buffer+IDX_NUM);
      printf("TYPE   : %s\n", buffer+IDX_TYPE);
      printf("MESG   : %s\n", buffer+IDX_MESSAGE);
      printf("CRC    : %s\n", buffer+IDX_CRC);
      printf("\n");
      
      handleMessage(buffer);
    }
  }
}

/**
initServer: creates a server and waits to accept the first client.
After accepting a client the server starts a thread to listen for incomming data.
*/
int initServer()
{
  int sockfd;
  
  struct sockaddr_in self;
  //char buffer[];
  
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
  {
    perror("Error trying to open socket");
    exit(2);
  }

  bzero(&self, sizeof(self));
  self.sin_family = AF_INET;
  self.sin_port = htons(PORT);
  self.sin_addr.s_addr = INADDR_ANY;
  
  if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
  {
    perror("socket--bind");
    exit(1);
  }


  if ( listen(sockfd, 20) != 0 )
  {
    perror("socket--listen");
    exit(3);
  }

  communicator = SERVER;

  
  //  while (1)
  //  {
    int clientfd;
    struct sockaddr_in client_addr;
    int addrlen=sizeof(client_addr);
    
    /*---accept a connection (creating a data pipe)---*/
    clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
    printf("%s:%d connected %d\n", (char*)inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), clientfd);
    pthread_t reader_thread;

    client_id = clientfd;
	    
    if(pthread_create(&reader_thread, NULL, waitForResponseServer, &clientfd))
      {
	perror("Error creating reader thread\n");
	exit(1);
      }
    printf("Started listener.\n");
    sleep(1);
    /*---Echo back anything sent---*/
    //send(clientfd, buffer, recv(clientfd, buffer, MAXBUF, 0), 0);
    
    /*---Close data connection---*/
    //close(clientfd);
    //}
  
  /*---Clean up (should never get here!)---*/
    // close(sockfd);
  return 0;
}

/**
initClient: creates a client socket to connect to the server. 
Starts a thread that will check for incomming data from the server.
*/
int initClient(){
  int socket_fd;
  //char packaged[FRAME_SIZE];
  //char buffer[FRAME_SIZE];
  
  //init socket
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_fd < 0){
    perror("Error opening socket");
    exit(1);
  }

  struct addrinfo hints, *servinfo, *p;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
  hints.ai_socktype = SOCK_STREAM;
  
  if ((rv = getaddrinfo("127.0.0.1", "6660", &hints, &servinfo)) != 0) //hard coded
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((socket_fd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1)
    {
      perror("socket");
      continue;
    }

    if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(socket_fd);
      //perror("connect");
      continue;
    }
    
    break; // if we get here, we must have connected successfully
  }
  
  server_id = socket_fd;
  communicator = CLIENT;
  
  pthread_t reader_thread;
  
  if(pthread_create(&reader_thread, NULL, waitForResponseServer, &server_id)){
    fprintf(stderr, "Error creating reader thread\n");
    exit(1);
  }

  sleep(1);
  return 0;
}


/**
shuffle: This function takes in an array and jumbles up its data randomly
*/
void shuffle(int *array, size_t n)
{
  if (n > 1)
  {
    size_t i;
    for (i = 0; i < n - 1; i++)
    {
      //size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      size_t j = i + rand() % n;
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

/**
physicalSend: This method is used by both the server and client. 
The function takes in a char * and determines if it will send the data
and whether it will be corrupted before sent. This message if sent will
then be recieved by the server or clients wairForResponse thread.
*/
void physicalSend(char *buffer, int size)
{
  int n; 
  int drop = (rand() % 100)+1;
  int corrupt = (rand() % 100)+1;
  if(drop > dropRate)
  {
    if(corrupt <= corruptRate)
    {
      shuffle((int*)buffer, FRAME_SIZE);
      printf("Corrupted packet.\n");
    }
    /*printf("SIZE   : %s\n", buffer+IDX_SIZE);
    printf("NUM    : %s\n", buffer+IDX_NUM);
    printf("TYPE   : %s\n", buffer+IDX_TYPE);
    printf("MESG   : %s\n", buffer+IDX_MESSAGE);
    printf("CRC    : %s\n", buffer+IDX_CRC);
    printf("\n");*/
    
    if(communicator == SERVER){
      n = write(client_id, buffer, FRAME_SIZE);
      printf("Server ");
    } else if (communicator == CLIENT){
      n = write(server_id, buffer, FRAME_SIZE);
      printf("Client ");
    }
    printf("socket write sent %d bytes\n", n);
  }
  else
    {
      printf("Dropped packet.\n");
    }
}
