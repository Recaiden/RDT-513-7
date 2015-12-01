//The application Client
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include "physicallayer.h"
#include "linkfunctions.h"

#define PACKET_SIZE 100
#define SIZE_FILE_MAX 104857600

struct timespec ts;
int receivingFile = 0;
FILE *file;

int transfer_file(int socket_fd, char* filename)
{
  char packaged[PACKET_SIZE];
  char buffer[PACKET_SIZE-5];
  
  FILE *fp;
  fp = fopen(filename, "rb"); 
  if(fp == NULL)
  { perror("Error opening file"); return 1; }
  
  while(1)
  {
    bzero(packaged, PACKET_SIZE);
    bzero(buffer, PACKET_SIZE-5);
    
    int nread = fread(buffer, 1, PACKET_SIZE-5, fp);

    if(nread > 0)
    {
      //printf("BUFFERING:%s\n", buffer);
      //sprintf(packaged, "%04d%s", friend_fd, buffer);

      //printf("SENDING: %s\n", buffer);
      write(socket_fd, buffer, strlen(buffer));
      nanosleep(&ts, NULL);
    }    
    if (nread < PACKET_SIZE-5)
    {
      if (feof(fp))
      {
      bzero(packaged, PACKET_SIZE);
      bzero(buffer, PACKET_SIZE-5);
      //sprintf(packaged, "%04d", friend_fd);
      sprintf(buffer, "/ENDF");
      strcat(packaged, buffer);
      write(socket_fd, packaged, strlen(packaged));
      nanosleep(&ts, NULL);
      printf("End of file\n");
      return 0;
      }
      if (ferror(fp))
      {
	perror("Error reading\n");
	// TODO better handling. Should send special message?
	return 1;
      }
      else
      {
	perror("Unknown error.");
	return 2;
      }
      printf("File transferred.");
      break;
    }
  }
}

int checkCommands(char *buffer){
	if(strstr(buffer, "/Hello") == buffer ||
		strstr(buffer, "/Sendfile") == buffer ||
		strstr(buffer, "/Getfile") == buffer ||
		strstr(buffer, "/Status") == buffer ||
		strstr(buffer, "/Goodbye") == buffer){
		return(1);
	}
	return(0);
}

int sendCommand(char *buffer){
	if(checkCommands(buffer) == 1){
		printf("command sent.\n");
	  dataLinkSend(buffer, strlen(buffer));
	} else {
		printf("not a valid command please try again.\n");
	}
	return(1);
}


int main(int argc, char *argv[]){
	if(argc < 3) {
		fprintf(stderr, "Please add drop and corrupt rates");
    	exit(0);
  	}
	setRates(atoi(argv[1]), atoi(argv[2]));

	initClient();
	//begin cleint
	printf("Welcome to the application please type a command: ");
	char buffer[PACKET_SIZE];
	while(1){
		bzero(buffer, PACKET_SIZE);
   		fgets(buffer, PACKET_SIZE, stdin);
   		if(checkCommands(buffer)){
   			sendCommand(buffer);
   		} else {
   			printf("This command does not exist please try again.\n"); // maybe put in a help command
					statDump();
   		}
	}
	return(1);

}
