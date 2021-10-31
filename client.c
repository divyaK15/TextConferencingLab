#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MESSAGE_SIZE 1024

void login(char* login_info);
void logout(char* logout_info);
void join_session(int session_id);
void leave_session();
void create_session(int session_id);
void list(); // make sure that the list of sessions is global if no arguments
void quit();
void text(char* buffer);



int main(int argc, char *argv[]){
    
    // program expects IP address and port number as an argument, exit if fewer than 3 arguments
    if(argc != 3){
        printf("Enter in the following format: deliver <server IP address> <server port number> \nExiting program.\n"); 
        exit(0);  
    }

    int port = atoi(argv[2]); 

    // create a TCP socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0){
        printf("Error, no socket found.\n");
    }

    // populate server address information 
    struct sockaddr_in serveraddress;
    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_port = htons(port); 
    serveraddress.sin_addr.s_addr = inet_addr(argv[1]); //https://pubs.opengroup.org/onlinepubs/009695399/functions/inet_addr.html
    memset(serveraddress.sin_zero, 0, sizeof(serveraddress.sin_zero));
    socklen_t sizeofserver = sizeof(serveraddress);


    // connecting with the server
    int connection = connect(socket_fd, (struct sockaddr*)&serveraddress, sizeofserver);
    if (connection == -1){
        printf("Uh oh.\n");
        exit(1);
    }
    /******* COMMANDS TO IMPLEMENT ********/
    /*
    /login <client ID> <password> <server-IP> <server-port>
        Log into the server at the given address and port. The IP address is specified in the dotted decimal format
    /logout
        Log out of the server, but do not exit the client. The client should return to the same state as when you started running it
    /joinsession <session ID> 
        Join the conference session with the given session ID
    /leavesession 
        Leave the currently established session
    /createsession <session ID> 
        Create a new conference session and join it
    /list 
        Get the list of the connected clients and available sessions
    /quit 
        Terminate the program
    <text> 
        Send a message to the current conference session. The message is sent after the newline
    */

    char buffer[MESSAGE_SIZE];
    // Practical System Programming with C: Pragmatic Example Applications in Linux and Unix-Based Operating Systems queue By Sri Manikanta Palakollu
    while(1){
        bzero(buffer, sizeof(buffer));
        printf("Enter the message for the server:\n");
        scanf("%[^\n]%*c", buffer);

        // end the connection of the message sent was "end"
        if ((strncmp(buffer, "end", 3)) == 0) {
           write(socket_fd, buffer, sizeof(buffer));
           printf("Client Exit.\n");
           break;
       }
       ssize_t bytes_sent = write(socket_fd, buffer, sizeof(buffer));
       if (bytes_sent >= 0){
           printf("Data sent to server successfully :)\n");
       }
       bzero(buffer, sizeof(buffer));
       // read response from server
       read(socket_fd, buffer, sizeof(buffer));
       printf("Data received from server: %s\n", buffer);
    }

    // close socket connection
    close(socket_fd);
    return 0;
}