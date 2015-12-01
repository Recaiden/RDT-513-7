CC=gcc
LDFLAGS=-pthread
CFLAGS=-c -Wall

all: server client

server: server.o link.o physical.o
	$(CC) $(LDFLAGS) appserver.o datalinklayer.o physicallayer.o -o appserver.exe

client: client.o link.o physical.o
	$(CC) $(LDFLAGS) appclient.o datalinklayer.o physicallayer.o -o appclient.exe -lrt

link.o: datalinklayer.c linkfunctions.h physicallayer.h
	$(CC) $(CFLAGS) datalinklayer.c

physical.o: physicallayer.c physicallayer.h linkfunctions.h
	$(CC) $(CFLAGS) physicallayer.c

server.o: appserver.c
	$(CC) $(CFLAGS) appserver.c

client.o: appclient.c
	$(CC) $(CFLAGS) appclient.c

clean:
	rm *.o appclient.exe appserver.exe
