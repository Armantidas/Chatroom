CC = gcc
CFLAGS = -I 

chatroom: client.o server.o
	$(CC) -o chatroom client.o server.o
