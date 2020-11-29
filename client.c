//client part

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_BUFFER 2048

void messagebox(char *name, int socketFd);
void messagecreat(char *result, char *name, char *msg);
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port);
void setNonBlock(int fd);
void interruptHandler(int sig);

static int socketFd;
char uname[20]; //user name
char hostip[50]; //server IP
char portnum[20]; //server port

int main()
{

    struct sockaddr_in serverAddr;
    struct hostent *host;
    long port;
    printf("\n-------------WELCOME CHAT ZONE--------------------\n ");
    printf("\n__________________________________________________________________________________________________\n");
    printf("*NOTE:- For Exit-/quite");

    printf("\n__LOGIN INFORMAYION__");
    printf("\nEnter user name:");
    scanf("%s",uname);
    printf("\nEnter the server IP:");
    scanf("%s",hostip);
    printf("\nEnter the server port:");
    scanf("%s",portnum);


    if((host = gethostbyname(hostip)) == NULL)
    {
        fprintf(stderr, "Host name fail\n");
        exit(1);
    }
    port = strtol(portnum, NULL, 0);
    if((socketFd = socket(AF_INET, SOCK_STREAM, 0))== -1)
    {
        fprintf(stderr, "Create socket faild\n");
        exit(1);
    }

   printf("\n---------------------------------------------------------\n");
   printf("\n------CHAT BOX--------\n");


    setupAndConnect(&serverAddr, host, socketFd, port);
    setNonBlock(socketFd);
    setNonBlock(0);

    //handle interrupt signal
    signal(SIGINT, interruptHandler);

    //chat box
    messagebox(uname, socketFd);
}

//Handle chat box message
void messagebox(char *name, int socketFd)
{
    fd_set clientFds;
    char chatMsg[MAX_BUFFER];
    char chatBuffer[MAX_BUFFER], msgBuffer[MAX_BUFFER];

    while(1)
    {
        //Reset the fd set each time since select() modifies it
        FD_ZERO(&clientFds);
        FD_SET(socketFd, &clientFds);
        FD_SET(0, &clientFds);
        if(select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) //wait for an available fd
        {
            for(int fd = 0; fd < FD_SETSIZE; fd++)
            {
                if(FD_ISSET(fd, &clientFds))
                {
                    if(fd == socketFd) //receive message from server
                    {
                        int numBytesRead = read(socketFd, msgBuffer, MAX_BUFFER - 1);
                        msgBuffer[numBytesRead] = '\0';
                        printf("%s", msgBuffer);
                        memset(&msgBuffer, 0, sizeof(msgBuffer));
                    }

		    //input from keyboard and send to server
                    else if(fd == 0)
                    {
                        fgets(chatBuffer, MAX_BUFFER - 1, stdin);
                        if(strcmp(chatBuffer, "/quite\n") == 0)
                            interruptHandler(-1); //Reuse the interruptHandler function to disconnect the client
                        else
                        {
                            messagecreat(chatMsg, uname, chatBuffer);
                            if(write(socketFd, chatMsg, MAX_BUFFER - 1) == -1)
                            perror("write failed:");
                            memset(&chatBuffer, 0, sizeof(chatBuffer));
                        }
                    }
                }
            }
        }
    }
}

//Concatenates the name with the message and puts it into result
void messagecreat(char *result, char *name, char *msg)
{
    memset(result, 0, MAX_BUFFER);
    strcpy(result, name);
    strcat(result, ": ");
    strcat(result, msg);
}

//Sets up the socket and connects
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port)
{
    memset(serverAddr, 0, sizeof(serverAddr));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    serverAddr->sin_port = htons(port);
    if(connect(socketFd, (struct sockaddr *) serverAddr, sizeof(struct sockaddr)) < 0)
    {
        perror("Server connection failed");
        exit(1);
    }
}

//Sets the fd to nonblocking
void setNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        perror("fcntl failed");

    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

//Notify the server when the client exits by sending "/exit"
void interruptHandler(int sig_unused)
{
    if(write(socketFd, "/quite\n", MAX_BUFFER - 1) == -1)
        perror("write failed: ");

    close(socketFd);
    exit(1);
}
