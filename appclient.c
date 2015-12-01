//The application Client
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "physicallayer.h"
#include "linkfunctions.h"

#define PACKET_SIZE 100
#define SIZE_FILE_MAX 104857600

struct timespec tstart={0,0}, tend={0,0};
int receivingFile = 0;
FILE *file;
struct timespec ts;
int transferingFile = 0;


int transfer_file(char* filename) {

  char buffer[PACKET_SIZE];
  
  FILE *fp;
  fp = fopen(filename, "rb"); 
  if(fp == NULL)
  { perror("Error opening file"); return 1; }
  
  while(1)
  {
    bzero(buffer, PACKET_SIZE);
    
    int nread = fread(buffer, 1, PACKET_SIZE, fp);

    if(nread > 0)
    {
      //printf("BUFFERING:%s\n", buffer);
      //sprintf(packaged, "%04d%s", friend_fd, buffer);
      //printf("SENDING: %s\n", buffer);
      sendCommand(buffer);
      dataLinkRecv(buffer);
      //write(socket_fd, buffer, strlen(buffer));
      nanosleep(&ts, NULL);
    }    
    if (nread < PACKET_SIZE)
    {
      if (feof(fp))
      {
     
      bzero(buffer, PACKET_SIZE);
      //sprintf(packaged, "%04d", friend_fd);
      sprintf(buffer, "/ENDF");
      sendCommand(buffer);
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
	} else if(transferingFile == 1){
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
   		if(strstr(buffer, "/Sendfile") == buffer){
   			sendCommand(buffer);
   			dataLinkRecv(buffer);
   			transferingFile = 1;
   			transfer_file("./small.txt");
   		} else if (strstr(buffer, "/Getfile") == buffer){
   			sendCommand(buffer);
   			dataLinkRecv(buffer);
   			if((buffer, "Sending file") == buffer){
   			strcpy(buffer, "send more of file");
			sendCommand(buffer);
			FILE *newFile;
			newFile = fopen("evensmaller.txt", "w+b"); 
				while(1){
					int size = dataLinkRecv(buffer);
					if((buffer, "/ENDF") == buffer) {
						fclose(newFile);
						break;
					}
					fwrite(buffer, 1, size, newFile);
					strcpy(buffer, "send more of file");
					sendCommand(buffer);
				}
			}
   		} else if(checkCommands(buffer)){
   			
   			clock_gettime(CLOCK_MONOTONIC, &tstart);
   			sendCommand(buffer);
   			printf("send command\n");
   			dataLinkRecv(buffer);
   			printf("received\n");
   		 	clock_gettime(CLOCK_MONOTONIC, &tend);

    		printf("Round trip in: %.5f seconds\n",
           ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));

   		} else {
   			printf("This command does not exist please try again.\n"); // maybe put in a help command
					statDump();
   		}
	}
	return(1);

}
