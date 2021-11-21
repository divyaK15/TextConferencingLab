#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "message.h"


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

char msg[500] = {'\0'};
void login(char* login_info);
void logout();
void joinSession(char* session_id);
void leaveSession(); 
void createSession(char* new_session_id); 
void list(); 
void quit(); 
void messageToString(unsigned int type/*, unsigned int size*/, unsigned char source[MAX_NAME], unsigned char data[MAX_DATA]/*, char* string*/);

void clear_message();

char client_name[100] = {'\0'};
int socket_fd;
message send_message;
struct sockaddr_in serveraddress;
pthread_mutex_t mutex;

void *recvmg(void *my_sock)
{
	int sock = *((int *)my_sock);
	int len;
	// client thread always ready to receive message
	while((len = recv(sock,msg,500,0)) > 0) {
		msg[len] = '\0';
		fputs(msg,stdout);
	}
}

int main(/*int argc,char *argv[]*/){
	pthread_t recvt;
	int len;
	
	char send_msg[500];
	struct sockaddr_in ServerIp;
	
	/*strcpy(client_name, argv[1]);*/

    // establish connection with server first, then can create threads for listening
	
	
	//creating a client thread which is always waiting for a message
	// pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
	
	//ready to read a message from console
    // include the different possible commands here
    
    //check if user is logged in 
    bool logged_in = false; 
	while(fgets(msg,500,stdin) > 0) {
		if((strncmp(msg, "/login", 6))== 0){
            login(msg); 
            logged_in = true; 
			pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
        }
        else if((strncmp(msg, "/logout", 7)) == 0){
            pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
            logout();
            printf("logout: \n");
        }
        else if((strncmp(msg, "/joinsession", 12)) == 0){
            if(!logged_in){
                printf("Please login first. \n"); 
            }
            else{
            pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
            joinSession(msg);
            printf("join session: \n");
            }
            
        }
        else if((strncmp(msg, "/leavesession", 13)) == 0){
            if(!logged_in){
                printf("Please login first. \n"); 
            }
            else{
            pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
            leaveSession();
            printf("leave session: \n");
            }
            
        }
        else if((strncmp(msg, "/list", 5)) == 0){
            if(!logged_in){
                printf("Please login first. \n"); 
            }
            else{
            pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
            list();
            printf("leave session: \n");
            }
        }
        else if((strncmp(msg, "/quit", 5)) == 0){
            quit();
            printf("quit session: \n");
            close(socket_fd);
            return 0;
        }
        else if((strncmp(msg, "/createsession", 14)) == 0){
            if(!logged_in){
                printf("Please login first. \n"); 
            }
            else{
            pthread_create(&recvt,NULL,(void *)recvmg,&socket_fd);
            createSession(msg);
            printf("create session: \n");
            }
        }
        else {
            strcpy(send_msg,client_name);
            strcat(send_msg,": ");
            strcat(send_msg,msg);
            len = write(socket_fd,send_msg,strlen(send_msg));
            
            if(len < 0) 
                printf("message not sent.\n");
        }

    }
        
		
	  
	//thread is closed
	pthread_join(recvt,NULL);
	close(socket_fd);
	return 0;
}
	
void messageToString(unsigned int type, /*unsigned int size,*/ unsigned char* source, unsigned char* data/*, char* string*/){
	pthread_mutex_lock(&mutex);

	send_message.type = type; 
    bzero(send_message.data, sizeof(send_message.data));
    strcpy(send_message.data,data); 
    send_message.size = sizeof(send_message.data);
    bzero(send_message.source, sizeof(send_message.source));
    strcpy(send_message.source,source); 
    char message_string[1000]; 
	bzero(message_string, 1000);

	printf("type: %u\nsize: %u\nsource: %s\ndata: %s\n", send_message.type, send_message.size, send_message.source, send_message.data);


    int message_string_temp = sprintf(message_string, "%u:%u:%s:%s",send_message.type, send_message.size, send_message.source, send_message.data);
    //memcpy(message_string, send_message.filedata, packetToSend.size);
    printf("message: %s\n",message_string);
	
    ssize_t send_return; 
    //send_return = send(socket_fd, &message_string, sizeof(message_string), 0); 
    // send_return = send(socket_fd, &message_string, sizeof(message_string), 0); 
    send_return = write(socket_fd,message_string,strlen(message_string));
    if(send_return < 0){
        printf("oof \n");
    }
    bzero(message_string, 1000);
	//string = message_string;
	pthread_mutex_unlock(&mutex);

}
void login(char* buffer){
    char* username; 
    char* password; 
    char* server_ip; 
    char* server_port; 
    char newString[10][30]; 
    int i,j,ctr; 

    socket_fd = socket(AF_INET, SOCK_STREAM,0);
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
    strcpy(client_name, username); 
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
        printf("Uh oh.\n");
        exit(1);
    }

	char* message_string;
	messageToString(LOGIN, username, password/*, message_string*/);
    bzero(buffer, sizeof(buffer)); 
    clear_message(); 
}

// implement logout
void logout(){
    //send logout control message and set boolean false on server side 
    //logout, username, NULL 
    close(socket_fd);
}

void joinSession(char* buffer){
    char* joinSessionID;
    char newString[10][30];  
    int i, j, ctr; 
    i=0; j=0; ctr=0; 
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

    joinSessionID = newString[1]; 
    printf("session ID: %s\n", joinSessionID); 
    messageToString(JOIN, client_name, joinSessionID); 
    clear_message();

}

void leaveSession(){
    printf("%s is leaving session. \n", client_name); 
    messageToString(LEAVE_SESS, client_name, ""); 
    clear_message();
}

void list(){
    //defined value, rest is NULL 
    printf("Listing users and avaialable sesisons: \n"); 
    messageToString(QUERY, "", ""); 
    clear_message();
}

void quit(){
    return 0; 
}

void createSession(char* buffer){
    char* createSessionID;
    char newString[10][30];  
    int i, j, ctr; 
    i=0; j=0; ctr=0; 
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

    createSessionID = newString[1]; 
    printf("session ID: %s\n", createSessionID); 
    messageToString(NEW_SESS, client_name, createSessionID);
    clear_message();
}

void clear_message(){
    send_message.type = -1;
    // size can stay as max data 
    bzero(send_message.data, MAX_DATA);
    // don't want to clear the source because it is the username
}