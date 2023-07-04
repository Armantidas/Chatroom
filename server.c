#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 1024
#define MAXCLIENTS 10

int findEmptyUser(int *c_sockets)
{
    for (int i = 0; i <  MAXCLIENTS; i++)
    {
        if (c_sockets[i] == -1)
        {
            return i;
        }
    }
    return -1;
}


int main(int argc, char **argv){
    //Getting the port from arguments and converts it from string to integer type
    uint16_t port = atoi(argv[1]);
    //Creating an IPV4 TCP socket
    int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    int clientSockets[MAXCLIENTS];
    fd_set readSet;
    unsigned int clientAddrLen;
    struct sockaddr_in clientaddr;

    char buffer[BUFFLEN];

    if (argc != 2)
    {
        fprintf(stderr, "./NAME PORT \n");
        return -1;
    }

    if ((port < 1) || (port > 65535))
    {
        fprintf(stderr, "Invalid port specified\n");
        return -1;
    }

    if (listeningSocket < 0)
    {
        fprintf(stderr, "Created socket is invalid\n");
        return -1;
    }

    //Bind the socket to host and port
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET, //IPV4 family
        .sin_addr.s_addr = htonl(INADDR_ANY), //The htonl function converts a u_long from host to TCP/IP network byte order (which is big-endian).
        .sin_port = htons(port) //Convert unsigned short int from host byte order to network byte order
    };

    if (bind (listeningSocket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0)
    {
        fprintf(stderr, "Can't bind listening socket\n");
        return -1;
    }

    if (listen(listeningSocket, 5) <0)
    {
        fprintf(stderr, "Error in listen\n");
        return -1;
    }

    for (int i = 0; i < MAXCLIENTS; i++)
    {
        clientSockets[i] = -1;
    }

    int clientId;

    //Used in select function to select would be able to check from max to lowest socket readiness for reading or writing
    int maxfd = 0;

    while(1)
    {
        FD_ZERO(&readSet);
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (clientSockets[i] != -1)
            {
                FD_SET(clientSockets[i], &readSet);
                maxfd = (clientSockets[i] > maxfd) ? clientSockets[i] : maxfd;
            }
        }

        FD_SET(listeningSocket, &readSet);
        maxfd = (listeningSocket > maxfd) ? listeningSocket : maxfd;

        int value = select(maxfd + 1, &readSet, NULL , NULL, NULL);
        if(value < 0)
        {
            fprintf(stderr, "Select function error\n");
            exit(1);
        }

        if (FD_ISSET(listeningSocket, &readSet))
        {
            clientId = findEmptyUser(clientSockets);
            if (clientId != -1)
            {
                //Resetting clientaddr
                memset(&clientaddr, 0, sizeof(clientaddr));
                clientAddrLen = sizeof(clientaddr);
                clientSockets[clientId] = accept(listeningSocket, (struct sockaddr*)&clientaddr, &clientAddrLen);
                if(clientSockets[clientId] == -1)
                {
                    fprintf(stderr, "Accept function error");
                }
                //Inet ntoa converts the address back from byte form to string so we can display it
                printf("Connected:  %s, Number: %d\n", inet_ntoa(clientaddr.sin_addr), clientId + 1);
            }
        }

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (clientSockets[i] != -1)
            {
                if (FD_ISSET(clientSockets[i], &readSet))
                {
                    memset(&buffer, 0, BUFFLEN);
                    int bytesReceived = recv(clientSockets[i], &buffer, BUFFLEN, 0);

                    //Fixes the problem where if a socket that has joined later disconnects the socket with the prior connection still functions
                    if(bytesReceived <= 0)
                    {
                        close(clientSockets[i]);
                        clientSockets[i] = -1;
                        break;
                    }


                    for (int j = 0; j < MAXCLIENTS; j++)
                    {
                        //Making sure we are not wasting resources on disconnected sockets
                        if (clientSockets[j] != -1)
                        {
                            //Number = amount of bytes sent by send function
                            int bytesSent = send(clientSockets[j], buffer, bytesReceived, 0);
                            //Indicates that a client has disconnected
                            if(bytesSent <= 0)
                            {
                                //Closing the socket and marking it as -1 so other sockets could connect
                                close(clientSockets[j]);
                                clientSockets[j] = -1;
                                break;

                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
