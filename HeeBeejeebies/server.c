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

typedef struct client_info{ 
    unsigned char username[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char password[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char current_session[MAX_NAME]; // username as visible to other users in the chat room
    int fd; 
    bool logged_in; 
} client_info; 

//Create a pointer to structs array, and initialize each element to null pointer 
//When new client connects, intitialize struct with client info 
// add the struct to the pointer array 

client_info* g_masterClientList[100] = {NULL}; 

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

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
							inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, INET6_ADDRSTRLEN), newfd);
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
                        for(j = 0; j <= fdmax; j++) {
                            char buf_cpy[MESSAGE_SIZE] = {'\0'};
                            message recv_message;
                            strcpy(buf_cpy, buf);
                            recv_message = convertStringToMessage(buf_cpy, MESSAGE_SIZE);
                            if (recv_message.type == LOGIN){
                                client_info current_client; 
                                strcpy(current_client.username, recv_message.source); 
                                strcpy(current_client.password,recv_message.data); 
                                current_client.logged_in = true; 
                                int i = 0; 
                                do{
                                    if(g_masterClientList[i] == NULL){
                                        g_masterClientList[i] = &current_client; 
                                        break; 
                                    }
                                    i++; 
                                }while(1); 
                                printf("control message received: login\n");
                            }
                            /*else if (recv_message.type == LOGOUT){
                                 printf("control message received: logout\n");
                            }*/
                            else if (recv_message.type == JOIN){
                                 printf("control message received: join\n");
                            }
                            else if (recv_message.type == LEAVE_SESS){
                                 printf("control message received: leave\n");
                            }
                            else if (recv_message.type == NEW_SESS){
                                 printf("control message received: create\n");
                            }
                            else if (recv_message.type == QUERY){
                                 printf("control message received: list\n");
                            }
                            else if (recv_message.type == EXIT){
                                 printf("control message received: quit\n");
                            }
                            else{
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
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}
