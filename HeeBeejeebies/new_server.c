/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "message.h"
#include <stdbool.h>

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
#define MAX_USERS 100

typedef struct client_info{ 
    unsigned char username[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char password[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char current_session[MAX_NAME]; // username as visible to other users in the chat room
    int fd; 
    bool logged_in; 
} client_info; 

void clear_recv_message(message* recv_message);
//Create a pointer to structs array, and initialize each element to null pointer 
//When new client connects, intitialize struct with client info 
// add the struct to the pointer array 

client_info* g_masterClientList[MAX_USERS] = {NULL}; 
bool sessionExists(); 
void printMasterClientList();

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    char* port = argv[1];
    
    if(argc != 2){
        printf("Enter in the following format: server <port number> \nExiting program.\n");   
        exit(0);   // https://stackoverflow.com/questions/9944785/what-is-the-difference-between-exit0-and-exit1-in-c/9944875
    }

    return 0; 
}



/************************************* Helper Functions **************************************/
bool sessionExists(char* sessionID){
    printf("checking if session exists\n");
    for(int i=0; i<MAX_USERS; i++){
        client_info* current_client = g_masterClientList[i];
        if(current_client != NULL){
            //client_info* current_client = g_masterClientList[i];
            if(strcmp(current_client->current_session,sessionID) == 0){
                printf("Current client is %s\n", current_client->username);
                printf("Session %s exists.\n", sessionID); 
                return true; 
            }
        }
        else{
            // session has not come up and we have iterated through all non-Null entries 
            printf("Session %s does not exist.\n", sessionID); 
            return false; 
        }
    }
    return false; 

}

void clear_recv_message(message* recv_message){
    recv_message->type = -1;
    recv_message->size = MAX_DATA;
    bzero(recv_message->source, MAX_NAME);
    bzero(recv_message->data, MAX_DATA);
}

void printMasterClientList(){
    int i = 0;
    for (i = 0; (i < MAX_USERS) && (g_masterClientList[i] != NULL); ++i){
        client_info* current_client = g_masterClientList[i];
        printf("Client #%d\n", i);
        printf("username: %s\n", current_client->username);
        printf("password: %s\n", current_client->password);
        printf("current session: %s\n", current_client->current_session);
        printf("file descriptor: %d\n", current_client->fd);
        printf("logged in? %d\n",current_client->logged_in);
    }
}

