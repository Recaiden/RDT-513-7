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

void checkCommands(char *buffer){
  //maybe for changing physical layer probabilities
}

int sendCommand(char *buffer){
  dataLinkSend(buffer, strlen(buffer));
  return(1);
}

int receiveCommand(char *buffer){
	if(strstr(buffer, "/Hello") == buffer){
		sendCommand(buffer);// change to say hi or something
	} else if ( strstr(buffer, "/Sendfile") == buffer ){

	} else if ( strstr(buffer, "/Getfile") == buffer ){

	} else if ( strstr(buffer, "/Status") == buffer ){

	} else if ( strstr(buffer, "/Goodbye") == buffer){
		
	}
	return(1);
}

int main(int argc, char *argv[]){
	if(argc < 3) {
		fprintf(stderr, "Please add drop and corrupt rates");
    	exit(0);
  	}
	setRates(argv[1], argv[2]);

	//begin cleint
	initServer();
	printf("Server has begun.");
	char buffer[PACKET_SIZE];
	while(1){
		//check for command comming in
   		checkCommands(buffer);
	}
	return(1);

}
