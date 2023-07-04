#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 1024

int main(int argc, char **argv)
{
    unsigned int port = atoi(argv[2]);
    //Creating an IPv4 TCP socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    //Server address structure
    struct sockaddr_in servaddr = {0};
    //Structure to place sockets in sets(looking if the socket is ready to read to ready to write)
    fd_set readSet;

    if(argc != 3)
    {
        fprintf(stderr,"./NAME IP PORT");
        exit(1);
    }

    //Check is the specified port is correct
    if((port < 1) || (port > 65535))
    {
        printf("Invalid port specified");
        exit(1);
    }

    //Checking if the socket is correct
    if(clientSocket < 0)
    {
        fprintf(stderr, "Can't create client socket");
        exit(1);
    }

    //Initializing the elements in sockaddr_in structure(https://linuxhint.com/sockaddr-in-structure-usage-c/)
    servaddr.sin_family = AF_INET; //The AF_INET address family is the address family for IPv4
    servaddr.sin_port = htons(port); //The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.

    //Adding the ip to the structure and checking if it is correct
    if(inet_aton(argv[1], &servaddr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid IP address specified");
        exit(1);
    }

    //Connecting to the server
    if(connect(clientSocket,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0)
    {
        fprintf(stderr, "Failed to connect to server");
        exit(1);
    }

    //Creating two buffers for reading data from server and sending data to server
    char recvBuffer[BUFFLEN] = {0};
    char sendBuffer[BUFFLEN] = {0};

    int byteNumber = 0;

    //Setting socket to non block socket(Multiple sockets can be active at the same time, synchronous I/O)
    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NONBLOCK);

    while(1)
    {
        //Initializing set to empty set
        FD_ZERO(&readSet);
        //Adding socket to set
        FD_SET(clientSocket, &readSet);
        //Adding standart input to set
        FD_SET(0, &readSet);

        //Select is used to monitor a set of file descriptors for readiness to perform I/O operations
        int result = select(clientSocket + 1, &readSet, NULL, NULL, NULL);
        if(result < 0)
        {
            printf("Select failed");
        }

        //Checking if socket is a member of set(ready to read data from server)
        if(FD_ISSET(clientSocket, &readSet))
        {
            memset(&recvBuffer, 0, BUFFLEN);
            int bytesRead = read(clientSocket, &recvBuffer, BUFFLEN);
            if(bytesRead == -1)
            {
                printf("Error in recv\n");
                break;
            }
            else if(bytesRead == 0)
            {
                printf("Server closed the connection\n");
                break;
            }
            printf("User sent: %s\n", recvBuffer);
        }

        //Checks if standard input is in readSet set
        else if (FD_ISSET(0, &readSet))
        {
            //Reads from stdin to sendBuffer
            int byteNumber = read(0, &sendBuffer, BUFFLEN);
            if(byteNumber == -1)
            {
                printf("Error in read\n");
                break;
            }
            //Writes from buffer to socket
            write(clientSocket, sendBuffer, byteNumber);
        }
    }
    close(clientSocket);
    return 0;
}
