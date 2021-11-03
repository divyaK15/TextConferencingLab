
#include <stdio.h>

#define MESSAGE_SIZE 1024
#define MAX_NAME 1024 
#define MAX_DATA 1024 


typedef struct message
{
    unsigned int type; 
    unsigned int size; 
    unsigned char source[MAX_NAME]; 
    unsigned char data[MAX_DATA]; 
} message;

