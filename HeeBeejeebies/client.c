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

//implement all the functions
//one client can join multuple sessins but not at once 
//if quit --> probably just exit while loop 
//in main have a while loop 
//read from buffer, determine command
//call corrsponding function 


#define LOGIN 15 
#define LO_ACK 20 
#define LO_NAK 25
#define EXIT 1000
#define JOIN 30 
#define JN_ACK 35 
#define JN_NAK 40 
#define LEAVE_SESS 50 
#define NEW_SESS 60 
#define NS_ACK 65 
#define MESSAGE 70 
#define QUERY 80 
#define OU_ACK 85 

int socket_fd;

void login(char* login_info);
void logout(char* logout_info);
void join_session(char* session_join);
void leave_session();
void create_session(char* session_create);
void list(); // make sure that the list of sessions is global if no arguments
void quit();
void text(char* buffer);
char password_client[100]; 



message send_message;

struct sockaddr_in serveraddress;




int main(int argc, char *argv[]){

    char user_input[20];
    
    // program expects IP address and port number as an argument, exit if fewer than 3 arguments
    // if(argc != 3){
    //     printf("Enter in the following format: deliver <server IP address> <server port number> \nExiting program.\n"); 
    //     exit(0);  
    // }

    // int port = atoi(argv[2]); 

    // create a TCP socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0){
        printf("Error, no socket found.\n");
    }

    // populate server address information 
    
    // serveraddress.sin_family = AF_INET; 
    // serveraddress.sin_port = htons(port); 
    // serveraddress.sin_addr.s_addr = inet_addr(argv[1]); //https://pubs.opengroup.org/onlinepubs/009695399/functions/inet_addr.html
    // memset(serveraddress.sin_zero, 0, sizeof(serveraddress.sin_zero));
    // socklen_t sizeofserver = sizeof(serveraddress);


    // connecting with the server
    // int connection = connect(socket_fd, (struct sockaddr*)&serveraddress, sizeofserver);
    // if (connection == -1){
    //     printf("Uh oh.\n");
    //     exit(1);
    // }
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
        if ((strncmp(buffer, "quit", 5)) == 0) {
           write(socket_fd, buffer, sizeof(buffer));
           printf("Client Exit.\n");
           break;
       }
        //login 
        else if((strncmp(buffer, "/login", 6))== 0){
            login(buffer); 
        }

        //logout
        else if((strncmp(buffer, "/logout", 7)) == 0){
            printf("logout: \n");
        }
        
        //joinsession
        else if((strncmp(buffer, "/joinsession", 12)) == 0){
            join_session(buffer); 
        }

        //leavesession
        else if((strncmp(buffer, "/leavesession", 13))== 0){
            
        }

        //createsession 
        else if((strncmp(buffer, "/createsession", 14)) == 0){
            create_session(buffer); 
        }

        //list 
        else if((strncmp(buffer, "/list", 5)) == 0){
            printf("join session: \n");
        }

        else{

       ssize_t bytes_sent = write(socket_fd, buffer, sizeof(buffer));
       if (bytes_sent >= 0){
           printf("Data sent to server successfully :)\n");
       }
       bzero(buffer, sizeof(buffer));
       // read response from server
       read(socket_fd, buffer, sizeof(buffer));
       printf("Data received from server: %s\n", buffer);
        }
    }

    // close socket connection
    close(socket_fd);
    return 0;
}

void login(char* buffer){
    char* username; 
    char* password; 
    char* server_ip; 
    char* server_port; 
    char newString[10][30]; 
    int i,j,ctr; 

    printf("buffer: %s\n", buffer); 
    j=0; ctr=0; 
    for(i=0; i<=(strlen(buffer)); i++){
        if(buffer[i]==' ' || buffer[i]=='\0'){
            newString[ctr][j]='\0'; 
            ctr++;
            j=0; 
        }
        else{
            newString[ctr][j] = buffer[i]; 
            j++; 
        }
    }
    username = newString[1]; 
    password = newString[2]; 
    server_ip = newString[3]; 
    server_port = newString[4]; 

    int server_port_int = atoi(server_port);

    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_port = htons(server_port_int); 
    serveraddress.sin_addr.s_addr = inet_addr(server_ip); //https://pubs.opengroup.org/onlinepubs/009695399/functions/inet_addr.html
    memset(serveraddress.sin_zero, 0, sizeof(serveraddress.sin_zero));
    socklen_t sizeofserver = sizeof(serveraddress);

    int connection = connect(socket_fd, (struct sockaddr*)&serveraddress, sizeofserver);
    if (connection == -1){
        perror("Uh oh.\n");
        exit(1);
    }

    send_message.type = LOGIN; 
    strcpy(send_message.data,password); 
    send_message.size = sizeof(password); 
    strcpy(send_message.source,username); 

    ssize_t send_return; 
    send_return = send(socket_fd, &send_message, sizeof(send_message), 0); 
    if(send_return < 0){
        printf("oof \n");
    }

    printf("username: %s\n", username); 
    printf("password: %s\n", password); 
    printf("server_ip: %s\n", server_ip); 
    printf("server_port: %s\n", server_port); 
    bzero(buffer, sizeof(buffer)); 
}

void join_session(char* buffer){
    int i,j,ctr; 
    char* join_session_id; 
    char join_string[10][30];
    j=0; ctr=0; 
    for(i=0; i<=(strlen(buffer)); i++){
        if(buffer[i]==' ' || buffer[i]=='\0'){
            join_string[ctr][j]='\0'; 
            ctr++;
            j=0; 
        }
        else{
            join_string[ctr][j] = buffer[i]; 
            j++; 
        }
    }
    join_session_id = join_string[1]; 

    printf("join session: %s\n", join_session_id);
    bzero(buffer, sizeof(buffer)); 
}

void create_session(char* buffer){
    int i,j,ctr; 
    char* create_session_id; 
    char create_string[10][30];
    j=0; ctr=0; 
    for(i=0; i<=(strlen(buffer)); i++){
        if(buffer[i]==' ' || buffer[i]=='\0'){
            create_string[ctr][j]='\0'; 
            ctr++;
            j=0; 
        }
        else{
            create_string[ctr][j] = buffer[i]; 
            j++; 
        }
    }
    create_session_id = create_string[1]; 
    printf("create session: %s\n", create_session_id);
}
