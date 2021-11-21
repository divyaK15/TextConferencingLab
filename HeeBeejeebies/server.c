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
// handling user commands
void login_command(message* recv_message, int fdnum);
bool join_command(message* recv_message);
// helper functions 
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


    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

	char remoteIP[INET_ADDRSTRLEN]; // this used to be INET6_ADDSTRLEN

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

	struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // what is AI PASSIVE
	if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
        // listener is the file descriptor
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
        // we can reuse local addresses
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
        printf("binded\n");

        // only going to the first one that binds
		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // for the message that we receive from the clients
    message recv_message;

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            bzero(buf, sizeof buf);
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
					newfd = accept(listener,
						(struct sockaddr *)&remoteaddr,
						&addrlen);

					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        clear_recv_message(&recv_message);
                        recv_message = convertStringToMessage(buf, MESSAGE_SIZE);
                        
                
                        if (recv_message.type == LOGIN){
                            printf("login request received.\n");
                            printf("Socket ID: %d\n",i);
                            login_command(&recv_message, i);
                        }
                        else if (recv_message.type == JOIN){
                            printf("join request received.\n");
                            join_command(&recv_message);
                        }
                        // logic for rest of the commands here
                        else {
                            // just basic text that needs to be sent out
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j != listener && j != i) {
                                        if (send(j, buf, nbytes, 0) == -1) {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        }
                        bzero(buf, sizeof buf);
                    }
                } // END handle data from client
                bzero(buf, sizeof buf);
            } // END got new incoming connection
            bzero(buf, sizeof buf);
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
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
    for (i = 0; ((i < MAX_USERS) && (g_masterClientList[i] != NULL)); ++i){
        client_info* current_client = g_masterClientList[i];
        
        printf("Client #%d\n", i);
        printf("username: %s\n", current_client->username);
        printf("password: %s\n", current_client->password);
        printf("current session: %s\n", current_client->current_session);
        printf("file descriptor: %d\n", current_client->fd);
        printf("logged in? %d\n",current_client->logged_in);
        printf("\n");
    }
}

void login_command(message* recv_message, int fdnum){
    // first checking the client info based on the control message
    client_info* current_client = malloc(sizeof(client_info));
    strcpy(current_client->username, recv_message->source); 
    strcpy(current_client->password, recv_message->data); 
    current_client->logged_in = true; 
    strcpy(current_client->current_session, "waiting_room"); // default session is the waiting room
    current_client->fd = fdnum; 

    // insert some logic here to authorize the user
    
    int i = 0; 

    do{
        // adding the client to the master list at the first available null entry
        if(g_masterClientList[i] == NULL){
            g_masterClientList[i] = current_client; 
            printf("inserted new client %s at iteration %d\n",current_client->username, i);

            break; 
        }
        // if the client name (which should be unique) is already in the global, make sure it's logged in = true
        else if (strcmp(g_masterClientList[i]->username, current_client->username) == 0){
            g_masterClientList[i]->logged_in = true;
            current_client->fd = fdnum; 
            printf("client %s already exists after %d iterations.\n", current_client->username, i);
            printf("master client list username: %s\n", g_masterClientList[i]->username);

            break;
        }
        i++; 

    }while(i < MAX_USERS); 

    free(current_client);
    
    
    // int client = 0;
    // for (client = 0; (client < MAX_USERS)&&(g_masterClientList[client] != NULL); ++client){
    //     if (g_masterClientList[client] == NULL){
    //         g_masterClientList[client] = &current_client; 
    //         break; 
    //     }
    //     else if (strcmp(g_masterClientList[client]->username, current_client.username) == 0){
    //         g_masterClientList[client]->logged_in = true;
    //         current_client.fd = fdnum; 
    //         printf("client %s already exists.\n", current_client.username);
    //         break;
    //     }
    // }

    printMasterClientList(); 
}

bool join_command(message* recv_message){
    return true;
}