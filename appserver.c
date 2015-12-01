// the Server application layer
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

#define PACKET_SIZE 100

int recievingFile = 0;
FILE *file;

void checkCommands(char *buffer){
  //maybe for changing physical layer probabilities
  //printf("Getting data from link layer\n");
	dataLinkRecv(buffer);
	receiveCommand(buffer);
}

int sendCommand(char *buffer){
  dataLinkSend(buffer, strlen(buffer));
  return(1);
}

int receiveCommand(char *buffer){
	if(strstr(buffer, "/Hello") == buffer){
		strcpy(buffer, "Hi How are you?");
	    sendCommand(buffer);
	} else if ( strstr(buffer, "/Sendfile") == buffer ){
		recievingFile = 1;

	} else if ( strstr(buffer, "/Getfile") == buffer ){

	} else if ( strstr(buffer, "/Status") == buffer ){
		strcpy(buffer, "Current Status:");
		sendCommand(buffer);
	} else if ( strstr(buffer, "/Goodbye") == buffer){
		strcpy(buffer, "See You Later!");
		sendCommand(buffer);
	} else if(strstr(buffer, "/Endl") == buffer){
		recievingFile = 0;
		fclose(file);
	} else if(recievingFile == 1){
		fwrite(buffer, 1, n, file);
	}
	return(1);
}

int main(int argc, char *argv[]){
	if(argc < 3) {
		fprintf(stderr, "Please add drop and corrupt rates\n");
    	exit(0);
  	}
	setRates(atoi(argv[1]), atoi(argv[2]));

	//begin cleint
	initServer();
	printf("Server has begun.\n");
	char buffer[PACKET_SIZE];
	char testbuffer[PACKET_SIZE];
	strcpy(testbuffer, "Testdatafromserver.");
	int n = 0;
	while(1){
		//check for command comming in
   		checkCommands(buffer);

		int rsend = (rand() % 100)+1;

		if(n == 0 && rsend > 95){
		  //n = dataLinkSend(testbuffer, strlen(testbuffer));
		}
	}
	return(1);

}
