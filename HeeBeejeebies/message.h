
#include <stdio.h>

#define MESSAGE_SIZE 1024
#define MAX_NAME 1024 
#define MAX_DATA 1024
#define MAX_USERS 100 


typedef struct message
{
    unsigned int type; // type of command
    unsigned int size; // size of message
    unsigned char source[MAX_NAME]; // username as visible to other users in the chat room
    unsigned char data[MAX_DATA]; // message to send
    
} message;

// message convertStringToMessage(char* msgString, int sizeOfMessage, int numEntries);
message convertStringToMessage(char* msgString);
void messageToString(int type, int size, char* source, char* data, char* str);
