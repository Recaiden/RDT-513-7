# RDT-513-7
CS 513 Class Project 2 for Team 7, 3-Layer Stack Chat Pair

Compiling the code:
You can type 'make' and it will compile both the client and the server programs.

Server:
1. To compile the server code navigate to the project folder using terminal.
2. Issue the command "make server" which will use the make file to create appserver.exe
3. To run the server type "./appserver.exe dropRate corruptionRate" into the terminal and a server instance will be created.

Client:
1. 1. To compile the client code navigate to the project folder using terminal.
2. Issue the command "make client" which will use the make file to create appclient.exe
3. To run a client type "./appclient.exe dropRate corruptionRate" into
the terminal and a client instance will be created as long as the server is running otherwise the client will fail to start successfully.
4. If successful the program will ask for the user to choose a nickname.
5. After choosing a nickname the client can type the command "/CHAT" which will signal the server that the client wants to be added to the client chatting queue.
6. When there is another client in the chat client queue, the server will pair the user with anoter client user providing them with their nickname.
7. From here the client can chat by sending messages, or use specific
commands defined below.

Both programs use common data layer and physical layer files, but they
have their own instances at runtime.  If you change 1 program from GBN
(the default mode) to Selective Repeat, or back, you must also change
the other, or packets will fail to be properly acknowledged.



Client speific commands:
1. /Hello sends to the server looking for a greeting response back
2. /Sendfile sends the file small.txt to the server
3. /Getfile gets the file small.txt from the server
4. /Status just gives the status which is just a message
5. /Goodbye sends a goodbye message back from the server