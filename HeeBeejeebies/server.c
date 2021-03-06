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
#define NS_NAK 66
#define MESSAGE 70 
#define QUERY 80 
#define QU_ACK 85 
#define PM 90
#define PM_ACK 95 
#define PM_NAK 96
#define REGISTER 100
#define RG_ACK 105
#define RG_NAK 106 
#define VIEW_ADMIN 110
#define VIEW_ACK 111
#define SET_ADMIN 120
#define SET_ACK 121
#define SET_NAK 122
#define KICK 130
#define KICK_ACK 131
#define KICK_NAK 132


typedef struct client_info{ 
    unsigned char username[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char password[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char current_session[MAX_NAME]; // username as visible to other users in the chat room
    int fd; 
    bool logged_in; 
    bool is_admin;
} client_info; 


//Create a pointer to structs array, and initialize each element to null pointer 
//When new client connects, intitialize struct with client info 
// add the struct to the pointer array 
//char sessionArr[MAX_USERS][MAX_USERS] = {"0"}; 

static client_info g_masterClientList[MAX_USERS]; 
int g_numEntries = 0;
// handling user commands
// void login_command(message* recv_message, int fdnum, char* source, char* data);
bool login_command(message* recv_message, int fdnum);
bool login_auth(message* recv_message, int fd);
bool register_client(message* recv_message, int fd);
void join_command(message* recv_message);
void leave_command(message* recv_message); 
void create_command(message* recv_message);
bool update_session(message* recv_message);
int view_admin(int senderIndex);
bool set_admin(message* recv_message, int senderIndex);
void kick_user(message* recv_message, int senderIndex);
void query_command();  
// helper functions 
bool sessionExists(); 
void printMasterClientList();
void initializeMasterClientList();
int identifyClientByFd(int fd);
int identifyClientByUsername(message* recv_message);
int firstClientInSession(char* session);
void uniqueSessions(int* unique_sessions, int size);
void clear_recv_message(message* recv_message);
void print_recv_message(message* recv_message);

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
    initializeMasterClientList();
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

    char buf[MAX_DATA];    // buffer for client data
    char buf_cpy[MAX_DATA];
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
    message recv_message, send_ack, send_message;
    char str_ack[MAX_DATA];
    char str_list[MAX_DATA];
    char str_list_cpy[MAX_DATA];

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        int senderIndex;
        int recvIndex;
        for(i = 0; i <= fdmax; i++) {
            bzero(buf, sizeof buf);
            senderIndex = identifyClientByFd(i);
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
                    // printf("Printing the list before receiving new data:\n");
                    // printMasterClientList();
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            g_masterClientList[senderIndex].logged_in = false;
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        // message recv_message;
                        clear_recv_message(&recv_message);
                        recv_message = convertStringToMessage(buf);
                        
                        print_recv_message(&recv_message);
                    
                        
                        /********** login ****************/
                        if (recv_message.type == LOGIN){
                            printf("login request received.\n");
                            printf("Socket ID: %d\n",i);
                            if(/*login_command(&recv_message, i)*/login_auth(&recv_message, i)){
                                send_ack.type = LO_ACK;
                                
                            }
                            else{
                                // ssize_t login_ACK; 
                                // login_ACK = send(i, "LO_NAK", sizeof("LO_NAK"),0);  
                                send_ack.type = LO_NAK;
                            }
                            messageToString(send_ack.type, 0, "", "", str_ack);
                            if (send(i, str_ack, strlen(str_ack), 0) < 0){
                                perror("Send Login ACK");
                            }
                            // clear_recv_message(&recv_message);
                        }
                        /*************** join **************/
                        else if (recv_message.type == JOIN){
                            printf("join request received.\n");
                            ssize_t join_ACK;
                            
                            if(sessionExists(recv_message.data)){ // session exists has print statements 

                                if (update_session(&recv_message)){
                                    printf("Updated session to %s successfully\n", recv_message.data);
                                    // printMasterClientList();
                                    send_ack.type = JN_ACK;
                                    /*
                                    char previousSession[50];
                                    if (g_masterClientList[senderIndex].is_admin){
                                        strcpy(previousSession, g_masterClientList[senderIndex].current_session);
                                        strcpy(g_masterClientList[senderIndex].current_session, "waiting_room");
                                        int newAdmin = firstClientInSession(previousSession);
                                        if (newAdmin != -1){
                                            g_masterClientList[newAdmin].is_admin = true;
                                        }
                                    }
                                    g_masterClientList[senderIndex].is_admin = false;
                                    */
                                }
                                else {
                                    // send nack
                                    send_ack.type = JN_NAK;
                                }
                            }
                            else{
                                    send_ack.type = JN_NAK;                                  
                            }
                            
                            messageToString(send_ack.type, 0, "", "", str_ack);
                            if (send(i, str_ack, strlen(str_ack), 0) < 0){
                                perror("Send Join ACK");
                            }
                            
                            
                        }
                        /*************** create ************/ 
                        else if(recv_message.type == NEW_SESS){
                            printf("create request recieved");
                            char previousSession[50];

                            if (g_masterClientList[senderIndex].is_admin){
                                strcpy(previousSession, g_masterClientList[senderIndex].current_session);
                                strcpy(g_masterClientList[senderIndex].current_session, "waiting_room");
                                int newAdmin = firstClientInSession(previousSession);
                                if (newAdmin != -1){
                                    g_masterClientList[newAdmin].is_admin = true;
                                }
                            }

                            if(!sessionExists(recv_message.data)){
                                // printf("Creating new session %s\n", recv_message.data); 
                                // create_command(&recv_message); 
                                
                                if (update_session(&recv_message)){
                                    send_ack.type = NS_ACK;
                                    g_masterClientList[senderIndex].is_admin = true;
                                }
                                else{
                                    send_ack.type = NS_NAK;
                                }
                            } 
                            else {
                                send_ack.type = NS_NAK;
                            }
                            messageToString(send_ack.type, 0, "", "", str_ack);
                            if (send(i, str_ack, strlen(str_ack), 0) < 0){
                                perror("Send Create ACK");
                            }
                        }
                        /************** leaving ************/ 
                        else if(recv_message.type == LEAVE_SESS){ 
                            char prevSession[50];
                            bool wasAdmin = g_masterClientList[senderIndex].is_admin;
                            strcpy(prevSession, g_masterClientList[senderIndex].current_session);
                            g_masterClientList[senderIndex].is_admin = false;
                            leave_command(&recv_message);
                            strcpy(g_masterClientList[senderIndex].current_session, "waiting_room");
                            if (wasAdmin){
                                // if the user that leaves the session is also the admin, want to set the admin to the first person in that session
                                // call function that returns index of first person in session
                                int newAdmin = firstClientInSession(prevSession);
                                if (newAdmin != -1){
                                    g_masterClientList[newAdmin].is_admin = true;
                                }
                                
                                // edge case --> there was no one else in the session (returns -1 and do nothing)
                            }
                            
                        }

                        /*********** query ************/

                        else if(recv_message.type == QUERY){
                            
                            query_command(str_list);
                            printf("Returned query string:\n %s", str_list);
                            messageToString(QU_ACK, 0, "", str_list, str_list_cpy);
                            // printf("Returned query string (after message):\n %s", str_list_cpy);
                            if (send(i, str_list_cpy, MAX_DATA, 0) == -1){
                                perror("Query");
    
                            }

                        }

                        /************** Private messaging ************/ 
                        else if(recv_message.type == PM){
                            printf("private_message request recieved\n"); 

                            int clientToSendInd = identifyClientByUsername(&recv_message);
                            int fdToSend;
                            char send_pm[MAX_DATA];
                            bzero(send_pm, MAX_DATA);

                            if(clientToSendInd < 0){
                                send_ack.type = PM_NAK; 
                            }
                            else{
                                fdToSend = g_masterClientList[clientToSendInd].fd; 
                                send_ack.type = PM_ACK; 
                            }
                            messageToString(send_ack.type, MAX_DATA, g_masterClientList[senderIndex].username, recv_message.data, send_pm);
                            send(g_masterClientList[clientToSendInd].fd, send_pm, MAX_DATA, 0);
                            bzero(send_pm, sizeof (send_pm));
                        }
                        /********************** View admin ****************************/
                        else if (recv_message.type == VIEW_ADMIN){
                            // the client that sent this --> what session they're in 
                            // iterate through the global list if == current session + check if they are admin
                            // if admin then return the index in the global that they correspond to
                            printf("Requested to view Admin for session %s\n", g_masterClientList[senderIndex].current_session);
                            int admin = view_admin(senderIndex);
                            char sendMessage[500];
                            messageToString(VIEW_ACK, MAX_DATA, g_masterClientList[admin].username, "", sendMessage);
                            if (send(i, sendMessage, MAX_DATA, 0) == -1){
                                perror("View Admin");
    
                            }

                        }
                        else if (recv_message.type == SET_ADMIN){
                            // the client that sent this --> what session they're in 
                            // check that the user exists
                            // check that they're logged in
                            // same session
                            // set the current admin's boolean to false and the new admin boolean to true
                            // if admin then return the index in the global that they correspond to
                            bool setAdmin = set_admin(&recv_message, senderIndex);
                            if (setAdmin){
                                send_message.type = SET_ACK;
                            }
                            else {
                                send_message.type = SET_NAK;
                            }
                            char sendMessage[500];
                            messageToString(send_message.type, MAX_DATA, g_masterClientList[setAdmin].username, "", sendMessage);
                            if (send(i, sendMessage, MAX_DATA, 0) == -1){
                                perror("Set Admin");
                            }
                            
                        }
                        else if (recv_message.type == KICK){
                            // the client that sent this --> what session they're in 
                            // check that the user that we want to kick out exists and is in the session
                            // change their session to waiting room BOOM
                            kick_user(&recv_message, senderIndex);
                        
                        }
                        // logic for rest of the commands here
                        else {
                            // just basic text that needs to be sent out
                            if(strcmp(g_masterClientList[senderIndex].current_session, "waiting_room") == 0){
                                continue;
                            }
                            printf("sending client has fd %d and current session %s\n", g_masterClientList[senderIndex].fd, g_masterClientList[senderIndex].current_session);
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j != listener && j != i) {
                                        recvIndex = identifyClientByFd(j);
                                        if (g_masterClientList[recvIndex].fd == -1) continue;
                                        if ((strcmp(g_masterClientList[recvIndex].current_session, g_masterClientList[senderIndex].current_session) == 0) && g_masterClientList[recvIndex].logged_in){
                                            printf("receiving client has fd %d and current session %s\n", g_masterClientList[recvIndex].fd, g_masterClientList[recvIndex].current_session);
                                            send_message.type = MESSAGE;
                                            send_message.size = MAX_DATA;
                                            strcpy(send_message.source, g_masterClientList[senderIndex].username);
                                            strcpy(send_message.data, buf);
                                        
                                            printf("buf before: %s\n", buf);
                                            messageToString(MESSAGE, MAX_DATA, g_masterClientList[senderIndex].username, recv_message.data, buf_cpy);
                                            printf("buf after: %s\n", buf_cpy);
                                            if (send(j, buf_cpy, nbytes, 0) == -1) {
                                                perror("send");
                                            }

                                        }
                                    }
                                }
                            }
                            bzero(buf_cpy, sizeof buf_cpy);
                            bzero(buf, sizeof buf);
                        }
                        // print_recv_message(&recv_message);
                        printMasterClientList();
                        
                    }
                    bzero(buf_cpy, sizeof buf_cpy);
                    bzero(buf, sizeof buf);
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0; 
}
/************************************ Command Functions *********************************************/

bool login_auth(message* recv_message, int fd){
    int clientIndex = identifyClientByUsername(recv_message);
    if (clientIndex == -1){
        printf("Client with username %s is not registered.\n",recv_message->source);
        return false;
    } 
    else if (strcmp(g_masterClientList[clientIndex].password, recv_message->data) != 0){
        printf("Incorrect password.\n");
        return false;    
    } 
    else{
        printf("User %s is authorized.\n", recv_message->source);
        // update the login information to be true
        g_masterClientList[clientIndex].logged_in = true;
        g_masterClientList[clientIndex].fd = fd;
        strcpy(g_masterClientList[clientIndex].current_session, "waiting_room");
        return true;
    } 
}

bool register_client(message* recv_message, int fd){
    // check if client can be registered by iterating through master list first (username must be unique)
    int regIndex = identifyClientByUsername(recv_message);
    if (regIndex != -1){
        printf("Username %s is taken\n", recv_message->source);
        return false;
    }
    else{
        // find the next available index in the global list
        // then insert the username and password given, followed by immediately loggining in, fd (should be given), and waiting_room
        for (int i = 0; i < MAX_USERS; i++){
            if (strcmp(g_masterClientList[i].username, "default_user") == 0){
                // replace everything then break
                strcpy(g_masterClientList[i].username, recv_message->source);
                strcpy(g_masterClientList[i].password, recv_message->data);
                strcpy(g_masterClientList[i].current_session, "waiting_room");
                g_masterClientList[i].fd = fd;
                g_masterClientList[i].logged_in = true;

                return true;
            }
        }
        printf("Not enough space to add new client.\n");
        return false;

    }

}

bool login_command(message* recv_message, int fdnum){
    printf("Printing the list before inserting.\n");
    printMasterClientList();
    
    // first checking the client info based on the control message
    client_info current_client;
    // strcpy(current_client->username, recv_message->source);
    // printf("Current client username: %s, Receive message source: %s\n", current_client->username, recv_message->source);
    // strcpy(current_client->password, recv_message->data); 
    // current_client->logged_in = true; 
    // strcpy(current_client->current_session, "waiting_room"); // default session is the waiting room
    // current_client->fd = fdnum; 

    // insert some logic here to authorize the user
    
    int i = 0; 

    do{
        
        // adding the client to the master list at the first available null entry
        
        if(strcmp(g_masterClientList[i].username, "default_user")==0){

            strcpy(current_client.username, recv_message->source);
            strcpy(current_client.password, recv_message->data); 
            current_client.logged_in = true; 
            strcpy(current_client.current_session, "waiting_room"); // default session is the waiting room
            current_client.fd = fdnum; 
            
            g_masterClientList[i] = current_client; 
            g_numEntries = i;
            printf("inserted new client %s at iteration %d\n",current_client.username, i);
            return true;  
        }
        // if the client name (which should be unique) is already in the global, make sure it's logged in = true
        else{
            client_info temp_client = g_masterClientList[i];
            if (strcmp(temp_client.username, recv_message->source) == 0){
                if (temp_client.logged_in){
                    printf("Client %s already logged in.\n",temp_client.username);
                    // send a nack 
                    return false;
                }
                else {
                    strcpy(g_masterClientList[i].username, recv_message->source); 
                    g_masterClientList[i].logged_in = true;
                    g_masterClientList[i].fd = fdnum; 
                    printf("client %s already exists after %d iterations.\n", current_client.username, i);
                    printf("master client list username: %s\n", temp_client.username);
                    return true; 
                }
                
            }
        }
        i++; 

    } while(i < MAX_USERS); 
    return false; 

}

// assuming this function is only called if the session does not exist
bool update_session(message* recv_message){
    int clientIndex = identifyClientByUsername(recv_message);
    if (g_masterClientList[clientIndex].logged_in == false){
        printf("Client not logged in. Please log in and try again. \n"); 
        return false;
    }
    else {
        strcpy(g_masterClientList[clientIndex].current_session, recv_message->data);
        printf("Client %s session ID is now %s\n", g_masterClientList[clientIndex].username, g_masterClientList[clientIndex].current_session);
        return true;
    }
}


void leave_command(message* recv_message){
    printf("leaving session function\n"); 
    int clientIndex = identifyClientByUsername(recv_message);
    strcpy(g_masterClientList[clientIndex].current_session, "waiting_room");

}

void query_command(char* final_list){
    int unique_sessions[MAX_USERS] = {-1}; 
    int sessArrayInd = 0;  
    // bool isUnique [MAX_USERS] = {false}; 
    char active_users[MAX_DATA]; 
    bzero(active_users, MAX_DATA); 
    char active_sessions[MAX_DATA]; 
    bzero(active_sessions, MAX_DATA); 
    strcpy(active_sessions, "List of active sessions: \n"); 
    strcpy(active_users, "List of usernames: \n"); 
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(g_masterClientList[i].username, "default_user")==0){
            break; 
        }
        else{
            if(g_masterClientList[i].logged_in){

                strcat(active_users, g_masterClientList[i].username); 
                strcat(active_users, "\n"); 
                int active_cnt = 0; 

                if(strcmp(g_masterClientList[i].current_session, "waiting_room") !=0){
                    // want to compare if the stirng of the current session is a substring of the active sessions string
                    if (strstr(active_sessions, g_masterClientList[i].current_session) == NULL){
                        strcat(active_sessions,  g_masterClientList[i].current_session);
                        strcat(active_sessions, "\n");
                        // strcat(active_sessions, "\n");
                    }
                }
            }
        }
    }
    /*
    for(int m = 0; m <MAX_USERS; m++){
        if(isUnique[m]){
            printf("username M: %s and session %s \n", g_masterClientList[m].username, g_masterClientList[m].current_session); 
            strcat(active_sessions, g_masterClientList[m].current_session); 
        }
    }*/

    // concatenate the username and session strings, then copy into final string
    strcat(active_users, active_sessions);
    strcpy(final_list, active_users);
    printf("Query string: %s\n", final_list); 
    return;
}

int view_admin(int senderIndex){
    // the client that sent this --> what session they're in 
    // iterate through the global list if == current session + check if they are admin
    // if admin then return the index in the global that they correspond to
    char currentSession[50];
    strcpy(currentSession, g_masterClientList[senderIndex].current_session);
    for (int i = 0; i < MAX_USERS; i++){
        // find the index of the user with the same name
        if ((strcmp(g_masterClientList[i].current_session, currentSession)==0) && g_masterClientList[i].is_admin) {
            return i;
        }
    }
    return -1;
}
bool set_admin(message* recv_message, int senderIndex){
    // the client that sent this --> what session they're in 
    if (!g_masterClientList[senderIndex].is_admin) return false;
    int newAdmin = identifyClientByUsername(recv_message);
    if (newAdmin == -1){
        printf("User %s does not exist.\n",recv_message->source);
        return false;
    }else if (!g_masterClientList[newAdmin].logged_in){
        return false;
    }else if (strcmp(g_masterClientList[newAdmin].current_session, g_masterClientList[senderIndex].current_session) != 0){
        printf("User %s is not in the same session.\n",recv_message->source);
        return false;
    }else {
        printf("User %s is the new admin\n", recv_message->source);
        g_masterClientList[senderIndex].is_admin = false;
        g_masterClientList[newAdmin].is_admin = true;
        return true;
    }

    // check that the user exists
    // check that they're logged in
    // same session
    // set the current admin's boolean to false and the new admin boolean to true
    // if admin then return the index in the global that they correspond to
}
void kick_user(message* recv_message, int senderIndex){
    // the client that sent this --> what session they're in 
    // check that the user that we want to kick out exists and is in the session
    // change their session to waiting room BOOM
    int screwThisGuy = identifyClientByUsername(recv_message);
    if (!g_masterClientList[senderIndex].is_admin) return;
    if (strcmp(g_masterClientList[screwThisGuy].current_session, g_masterClientList[senderIndex].current_session) == 0){
        strcpy(g_masterClientList[screwThisGuy].current_session, "waiting_room");
    }

}


/************************************* Helper Functions **************************************/
bool sessionExists(char* sessionID){
    printf("checking if session exists\n");
    printf("session ID to check: %s\n", sessionID); 
    for(int i=0; i<MAX_USERS; i++){
        client_info current_client = g_masterClientList[i];
        if (strcmp(g_masterClientList[i].username, "default_user") ==0){
            // we have iterated through all registered clients without finding the session
            return false;
        }
        else {
            if(!g_masterClientList[i].logged_in) {
                // we don't want to consider it as a potential session
                continue;
            }
            else{
                if(strcmp(current_client.current_session,sessionID) == 0){
                    printf("Current client is %s\n", current_client.username);
                    printf("Session %s exists.\n", sessionID); 
                    return true; 
                 }
            }


        }
    }
    return false; 

}

// void usersInSession(char* sessionID, int* array);
void uniqueSessions(int* unique_sessions, int size){
    for (int a = 0; a < size; a++){
        unique_sessions[a] = -1;
    }
    char strSession[500];
    bzero(strSession, 500);
    int insertIndex = 0;
    bool unique = true;
    for (int i = 0; i < MAX_USERS; i++){
        // check the session of the global at that index
        if (!g_masterClientList[i].logged_in) continue;
        strcpy(strSession, g_masterClientList[i].current_session);
        // if (!unique)
        for (int j = 0 ; j < size; j++){
            if((unique_sessions[j] == -1) && (strcmp(strSession, "waiting_room") != 0)){
                // insert it here
                unique_sessions[j] = i;
                printf("inserted %s here bitches user %d --> %d (index of unique one)\n",strSession, i, j);
                break;
            } 
            else if (unique_sessions[j] == -1) continue;
            else if ((unique_sessions[j] != -1) && (strcmp(strSession, g_masterClientList[unique_sessions[j]].current_session) == 0)){
                // they are the same 
                // do not insert this in the list
                unique = false;
                printf("kill me\n");
                break;
            }
        }

    }
}


void clear_recv_message(message* recv_message){
    recv_message->type = -1;
    recv_message->size = MAX_DATA;
    bzero(recv_message->source, MAX_NAME);
    bzero(recv_message->data, MAX_DATA);
}

void print_recv_message(message* recv_message){
    printf("Received at address %p\n", recv_message);
    printf("Receieved type: %d\n", recv_message->type);
    printf("Received source: %s\n", recv_message->source);
    printf("Received data: %s\n", recv_message->data);
}

void printMasterClientList(){
    int i = 0;
    for (i = 0; i < MAX_USERS ; ++i){
        if(strcmp(g_masterClientList[i].username, "default_user")==0){
            break; 
        }else if (!g_masterClientList[i].logged_in){
            continue;
        }
        client_info* current_client = &g_masterClientList[i];
        
        printf("Client #%d at address %p\n", i, &g_masterClientList[i]);
        printf("username: %s\n", current_client->username);
        printf("password: %s\n", current_client->password);
        printf("current session: %s\n", current_client->current_session);
        printf("file descriptor: %d\n", current_client->fd);
        printf("logged in? %d\n",current_client->logged_in);
        printf("admin? %d\n", current_client->is_admin);
        printf("\n");
    }
}

void initializeMasterClientList(){
    
    // initialize client stefan
    strcpy(g_masterClientList[0].username,"stefan");
    strcpy(g_masterClientList[0].password,"redpandas123");

    // client marc
    strcpy(g_masterClientList[1].username,"morc");
    strcpy(g_masterClientList[1].password,"bananaphone");

    // client kirti
    strcpy(g_masterClientList[2].username,"kirti");
    strcpy(g_masterClientList[2].password,"password");

    // client tenzin
    strcpy(g_masterClientList[3].username,"tenzin");
    strcpy(g_masterClientList[3].password,"dime$");

    
    printf("Initialize Master function: \n"); 
    for (int i = 0; i < MAX_USERS; ++i){
        if ( i > 3){
            strcpy(g_masterClientList[i].username, "default_user"); 
            strcpy(g_masterClientList[i].password, "default_pass"); 
        }
        strcpy(g_masterClientList[i].current_session, "default_session"); 
        g_masterClientList[i].fd = -1; 
        g_masterClientList[i].logged_in = false; 
        g_masterClientList[i].is_admin = false;
    }
}

int identifyClientByFd(int fd){
    int recv_fd;
    for (recv_fd = 0; recv_fd < MAX_USERS; recv_fd++){
        if (g_masterClientList[recv_fd].fd == fd){
            return recv_fd;
        }
    }
    return -1;
}

int identifyClientByUsername(message* recv_message){
    for (int i = 0; i < MAX_USERS; i++){
        // find the index of the user with the same name
        if (strcmp(g_masterClientList[i].username, recv_message->source)==0){
            return i;
        }
    }
    return -1;
}

int firstClientInSession(char* session){
    for (int i = 0; i < MAX_USERS; i++){
        // find the index of the user with the same name
        if (strcmp(g_masterClientList[i].username, session)==0){
            return i;
        }
    }
    return -1;
}



