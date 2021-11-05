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


#include "message.h"


#include <pthread.h>



int main(int argc, char *argv[]){

    int port = atoi(argv[1]); 
    
    if(argc != 2){
        printf("Enter in the following format: server <port number> \nExiting program.\n");   
        exit(0);   // https://stackoverflow.com/questions/9944785/what-is-the-difference-between-exit0-and-exit1-in-c/9944875
    }
    // create a TCP socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
        printf("Error, no socket found.\n");
        exit(1);
    }

        // populating server address information
    struct sockaddr_in server_sock_addr;
    server_sock_addr.sin_family = AF_INET; 
    server_sock_addr.sin_port = htons(port); 
    server_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // setting sin_zero to zero
    // https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
    memset(server_sock_addr.sin_zero, 0, sizeof(server_sock_addr.sin_zero));

    // bind the socket to the specified address (IP + port number)
    int bind_status = bind(socket_fd, (struct sockaddr*)&server_sock_addr, sizeof(server_sock_addr));
    if (bind_status ==-1){
        printf("Socket failed to bind.\n");
        exit(1);
    }

    // Assuming we want a maximum of 10 connections in the queue
    int listen_status = listen(socket_fd, 10);
    if (listen_status == -1){
        printf("Error: server is deaf\n");
        exit(1);
    }
    else{
        printf("Server is listening for new connections.\n");
    }

    // initialize struct to receive client info
    struct sockaddr_in client_info; 
    socklen_t client_length = sizeof(client_info); 
    
    int connection = accept(socket_fd, (struct sockaddr*) &client_info, &client_length);
    if (connection == -1){
        printf("Server is unable to accept data from client.\n");
        exit(1);
    }

    char buffer[MESSAGE_SIZE];

    // communication with client
    // Practical System Programming with C: Pragmatic Example Applications in Linux and Unix-Based Operating Systems queue By Sri Manikanta Palakollu
    while(1){
       message mess_recvd; 
       bzero(buffer, MESSAGE_SIZE);
       read(connection, buffer, sizeof(buffer));
       ssize_t recv_return; 
       recv_return = recv(socket_fd, &mess_recvd, sizeof(mess_recvd), 0); 
       if(recv_return < 0){
           printf("no no recv \n"); 
       }


       // ends the connection if it receives "end" from client
       if (strncmp("quit", buffer, 3) == 0) {
           printf("Client Exited.\n");
           printf("Server is exiting.\n");
           break;
       }
       printf("Data received from client: %s\n", buffer);
       bzero(buffer, MESSAGE_SIZE);
       printf("Enter the message to send to the client: \n");
       scanf("%[^\n]%*c", buffer);
        // Sending the data to the server by storing the number of bytes transferred in bytes variable
       ssize_t bytes_sent = write(connection, buffer, sizeof(buffer));
       // If the number of bytes is >= 0 then the data is sent successfully
       if(bytes_sent >= 0){
           printf("Data successfully sent to the client :)\n");
       }
   }
   // Closing the Socket Connection
   close(socket_fd);


    return 0;
}