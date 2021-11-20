// /*
// ** selectserver.c -- a cheezy multiperson chat server
// */

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <netdb.h>
// #include "message.h"
// #include <stdbool.h>

// #define LOGIN 15 
// #define LO_ACK 20 
// #define LO_NAK 25
// #define EXIT 1000
// #define JOIN 30 
// #define JN_ACK 35 
// #define JN_NAK 40 
// #define LEAVE_SESS 50 
// #define NEW_SESS 60 
// #define NS_ACK 65 
// #define MESSAGE 70 
// #define QUERY 80 
// #define OU_ACK 85 
// #define MAX_USERS 100

// typedef struct client_info{ 
//     unsigned char username[MAX_NAME]; // username as visible to other users in the chat room
//     unsigned char password[MAX_NAME]; // username as visible to other users in the chat room
//     unsigned char current_session[MAX_NAME]; // username as visible to other users in the chat room
//     int fd; 
//     bool logged_in; 
// } client_info; 
// void clear_recv_message(message* recv_message);
// //Create a pointer to structs array, and initialize each element to null pointer 
// //When new client connects, intitialize struct with client info 
// // add the struct to the pointer array 

// client_info* g_masterClientList[MAX_USERS] = {NULL}; 
// bool sessionExists(); 
// void printMasterClientList();

// // get sockaddr, IPv4 or IPv6:
// void *get_in_addr(struct sockaddr *sa)
// {
// 	if (sa->sa_family == AF_INET) {
// 		return &(((struct sockaddr_in*)sa)->sin_addr);
// 	}

// 	return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }

// int main(int argc, char *argv[])
// {
//     char* port = argv[1];
    
//     if(argc != 2){
//         printf("Enter in the following format: server <port number> \nExiting program.\n");   
//         exit(0);   // https://stackoverflow.com/questions/9944785/what-is-the-difference-between-exit0-and-exit1-in-c/9944875
//     }

//     return 0; 
// }



// /************************************* Helper Functions **************************************/
// bool sessionExists(char* sessionID){
//     printf("checking if session exists\n");
//     for(int i=0; i<MAX_USERS; i++){
//         client_info* current_client = g_masterClientList[i];
//         if(current_client != NULL){
//             //client_info* current_client = g_masterClientList[i];
//             if(strcmp(current_client->current_session,sessionID) == 0){
//                 printf("Current client is %s\n", current_client->username);
//                 printf("Session %s exists.\n", sessionID); 
//                 return true; 
//             }
//         }
//         else{
//             // session has not come up and we have iterated through all non-Null entries 
//             printf("Session %s does not exist.\n", sessionID); 
//             return false; 
//         }
//     }
//     return false; 

// }

// void clear_recv_message(message* recv_message){
//     recv_message->type = -1;
//     recv_message->size = MAX_DATA;
//     bzero(recv_message->source, MAX_NAME);
//     bzero(recv_message->data, MAX_DATA);
// }

// void printMasterClientList(){
//     int i = 0;
//     for (i = 0; (i < MAX_USERS) && (g_masterClientList[i] != NULL); ++i){
//         client_info* current_client = g_masterClientList[i];
//         printf("Client #%d\n", i);
//         printf("username: %s\n", current_client->username);
//         printf("password: %s\n", current_client->password);
//         printf("current session: %s\n", current_client->current_session);
//         printf("file descriptor: %d\n", current_client->fd);
//         printf("logged in? %d\n",current_client->logged_in);
//     }
// }

//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
	
#define TRUE 1
#define FALSE 0
#define PORT 8888
	
int main(int argc , char *argv[])
{
	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[30] ,
		max_clients = 30 , activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;
		
	char buffer[1025]; //data buffer of 1K
		
	//set of socket descriptors
	fd_set readfds;
		
	//a message
	char *message = "ECHO Daemon v1.0 \r\n";
	
	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
	}
		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
		sizeof(opt)) < 0 )
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
		
	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);
		
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");
		
	while(TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);
	
		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;
			
		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++)
		{
			//socket descriptor
			sd = client_socket[i];
				
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);
				
			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}
	
		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
			
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d
				\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
				(address.sin_port));
		
			//send new connection greeting message
			if( send(new_socket, message, strlen(message), 0) != strlen(message) )
			{
				perror("send");
			}
				
			puts("Welcome message sent successfully");
				
			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if( client_socket[i] == 0 )
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n" , i);
						
					break;
				}
			}
		}
			
		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
				
			if (FD_ISSET( sd , &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valread = read( sd , buffer, 1024)) == 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" ,
						inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
				}
					
				//Echo back the message that came in
				else
				{
					//set the string terminating NULL byte on the end
					//of the data read
					buffer[valread] = '\0';
					send(sd , buffer , strlen(buffer) , 0 );
				}
			}
		}
	}
		
	return 0;
}

