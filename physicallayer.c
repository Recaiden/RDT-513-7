
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include "linkfunctions.h"
#include "physicallayer.h"

int init = 0;

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
void * waitForResponse(void *socket){
	int n;
	int * socket_fd = (int *)socket;
  	char chat_buffer[FRAME_SIZE];
  	while(1){
	    //read server response
	    bzero(chat_buffer, FRAME_SIZE);
	    n = read((* socket_fd), chat_buffer, FRAME_SIZE);
	    //printf("RECEIVED: %s\n", chat_buffer);
	    if(n < 0){
	      sleep(1); //sleep some time while waiting for a message
	    } else {
	    	handleMessage(chat_buffer);
	    }
	}
}

/**
initServer: creates a server and waits to accept the first client.
After accepting a client the server starts a thread to listen for incomming data.
*/
int initServer(){
  int socket_fd, port_num, client_len;
  fd_set fd_master_set, read_set;
  //char buffer[FRAME_SIZE];
  struct sockaddr_in server_addr, client_addr;
  int n, i, number_sockets = 0;
  int client_fd[MAX_SOCKETS];


  //init fd to socket type
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_fd < 0){
    perror("Error trying to open socket");
    exit(1);
  }

  // Only run once.
  if(init == 0)
  {
    init = 1;
  
    //init socket
    bzero((char *) &server_addr, sizeof(server_addr));  //zero out addr
    port_num = PORT; // set port number
  
    //server_addr will contain address of server
    server_addr.sin_family = AF_INET; // Needs to be AF_NET
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // contains ip address of host 
    server_addr.sin_port = htons(port_num); //htons converts to network byte order
  
    int yes = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) { 
      perror("setsockopt"); 
      exit(1); 
    }  
  
    //bind the host address with bind()
    if(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
      perror("Error Binding");
      exit(1);
    }
    printf("Server listening for sockets on port:%d\n", port_num);
    
    //listen for clients
    int n = listen(socket_fd, MAX_SOCKETS);
    printf("Listen result %d\n", n);
    client_len = sizeof(client_addr); // set client length
  }
  
  FD_ZERO(&fd_master_set);
  FD_SET(socket_fd, &fd_master_set);

  communicator = SERVER;

  
  while(1){
    
    read_set = fd_master_set;
    n = select(FD_SETSIZE, &read_set, NULL, NULL, NULL);
    if(n < 0)
    { perror("Select Failed"); exit(1); }
    
    for(i = 0; i < FD_SETSIZE; i++)
    {
      if(FD_ISSET(i, &read_set))
      {
		  if(i == socket_fd)
		  {
		    if(number_sockets < MAX_CLIENTS)//MAX_SOCKETS)
		    {
		      //accept new connection from client
		      client_fd[number_sockets] = 
		        accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
		      if(client_fd[number_sockets] < 0)
		      { perror("Error accepting client"); exit(1); }
		      printf("Client accepted.\n");
		      FD_SET(client_fd[number_sockets], &fd_master_set);
		      number_sockets++;
		      break;
    	  }
  		}
      }
    }
  }

  pthread_t reader_thread;
  
  if(pthread_create(&reader_thread, NULL, waitForResponse, &socket_fd)){
    fprintf(stderr, "Error creating reader thread\n");
    exit(1);
  }

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
  
  if(pthread_create(&reader_thread, NULL, waitForResponse, &server_id)){
    fprintf(stderr, "Error creating reader thread\n");
    exit(1);
  }

  return 0;
}


/**
shuffle: This function takes in an array and jumbles up its data randomly
*/
void shuffle(int *array, size_t n){
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
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
void physicalSend(char *buffer)
{
  int n; 
  int drop = (rand() % 100)+1;
  int corrupt = (rand() % 100)+1;
  if(drop > dropRate){
    if(corrupt <= corruptRate){
      shuffle(buffer, strlen(buffer));
    }
    if(communicator == SERVER){
      n = write(4, buffer, strlen(buffer));
    } else if (communicator == CLIENT){
      n = write(server_id, buffer, strlen(buffer));
    }
  } 
}







