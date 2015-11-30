CC=gcc
LDFLAGS=-pthread
CFLAGS=-c -Wall

all: server client appclient

server: server.o link.o physical.o
	$(CC) $(LDFLAGS) server.o link.o physical.o -o appserver.exe

client: client.o link.o physical.o
	$(CC) $(LDFLAGS) client.o link.o physical.o -o appclient.exe

link.o: datalinklayer.c linkfunctions.h
	$(CC) $(CFLAGS) datalinklayer.c

physical.o: physicallayer.c physicallayer.h linkfunctions.h
	$(CC) $(CFLAGS) physicallayer.c


server.o: appserver.c
	$(CC) $(CFLAGS) appserver.c

client.o: appclient.c
	$(CC) $(CFLAGS) appclient.c

clean:
	rm *.o appclient.exe appserver.exe appclient appserver
