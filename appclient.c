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
   		}
	}
	return(1);

}
