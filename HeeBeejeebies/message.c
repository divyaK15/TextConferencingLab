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
#include <regex.h>
#include "message.h"

#define MAX_SRC 25




message convertStringToMessage2(char* msgString, int sizeOfMessage, int numEntries){
    
    message msgStruct;
    /*if (numEntries > 0){
        strcpy(msgStruct.data,"default");
        strcpy(msgStruct.source, "default");
        msgStruct.type = -1;
        msgStruct.size  = MESSAGE_SIZE;

        // printf("Returning message struct with funky data \n");
        return msgStruct;

    }*/
    

    
    // iterate through string and when we reach : its the next element of the struct
    int stringLength = sizeOfMessage;
    int colonCount = 0; // hey
    int colonIndex = 0;
    int colonPrevious = 0;
    char* msgHeaders[5]; 
    int msgHeaderIndex = 0; 

    //the packet's fields should all be divided into parts of the array (five total)
    for (int i = 0; i < stringLength; i++){
        if (msgString[i] == ':'){
            colonCount++;
            colonIndex = i;
            //printf("strlength %d\n", stringLength);
            if (colonCount == 1){
                msgHeaders[0] = malloc(sizeof(char) * (i - colonPrevious));
                // strncpy(msgHeaders[0], msgString + colonPrevious, i);
                bzero(msgHeaders[0], i - colonPrevious);
                memcpy((char*)msgHeaders[0], msgString + colonPrevious, i - colonPrevious);
                colonPrevious = colonIndex; 
                msgStruct.type = atoi((char*)msgHeaders[0]);
                // free(msgHeaders[0]);
                //printf("user = %u\n", packetStruct.total_frag);
                printf("convert string to message -- type at address %p or %p\n", msgHeaders[0], &msgStruct.type);
            }
            else if (colonCount == 2){
                msgHeaders[1] = malloc(sizeof(char) * (i - colonPrevious));
                bzero(msgHeaders[1], i - colonPrevious);
                memcpy(msgHeaders[1], msgString + colonPrevious + 1, i - colonPrevious - 1 );
                colonPrevious = colonIndex; 
                msgStruct.size = atoi(msgHeaders[1]);
                // free(msgHeaders[1]);
                //printf("frag no: %u \n",packetStruct.frag_no);
                printf("convert string to message -- size at address %p or %p\n", msgHeaders[1], &msgStruct.size);
            }
            else if (colonCount == 3){
                msgHeaders[2] = malloc(sizeof(char) * (i - colonPrevious));
                bzero(msgHeaders[2], i - colonPrevious);
                //memcpy(msgHeaders[2], msgString + colonPrevious + 1, i - colonPrevious -1 );
                strncpy(msgHeaders[2], msgString + colonPrevious + 1, i - colonPrevious -1); 
                colonPrevious = colonIndex; 
                strcpy(msgStruct.source, msgHeaders[2]); // source is the username in this case, character array
                // free(msgHeaders[2]);
                //printf("size: %u \n",packetStruct.size);
                printf("convert string to message -- source at address %p or %p\n", msgHeaders[2], &msgStruct.source);
            }
            else if(colonCount == 4){   
                msgHeaders[3] = malloc(sizeof(char) * (i - colonPrevious));
                bzero(msgHeaders[3], i - colonPrevious);
                memcpy(msgHeaders[3], msgString + colonPrevious + 1, i - colonPrevious - 1);
                colonPrevious = colonIndex; 
                // strcpy(msgStruct.data, msgHeaders[3]);
                // free(msgHeaders[3]);
                //printf("filename: %s\n",packetStruct.filename);
                printf("convert string to message -- data at address %p or %p\n", msgHeaders[3], &msgStruct.data);
            }         
        }
    }
    

    // strcpy(msgStruct.data, msgString + colonPrevious + 1);
    // strcpy(msgStruct.data, msgString + colonPrevious + 1);
    // printf("convert string to message -- data at address %p\n", &msgStruct.data);

    memcpy(msgStruct.data, msgString + colonPrevious + 1, msgStruct.size); // check this one
    //printf("filedata: %s \n",packetStruct.filedata);

    return msgStruct;
}

message convertStringToMessage(char* msgString){
    
    message msgStruct;
   
    // iterate through string and when we reach : its the next element of the struct
    int stringLength = strlen(msgString);
    int colonCount = 0; // hey
    int colonIndex = 0;
    int colonPrevious = 0;
    char* msgSource = malloc(sizeof (char)*(MAX_NAME)); 
    char* msgData= malloc(sizeof (char)*(MAX_DATA)); 
    char* msgType= malloc(sizeof (char)*(4)); 
    char* msgSize= malloc(sizeof (char)*(4)); 
    
    int msgHeaderIndex = 0; 

    bzero(msgSource, MAX_NAME); 
    bzero(msgData, MAX_DATA);
    bzero(msgType, 4);
    bzero(msgSize, 4);
    

    //the packet's fields should all be divided into parts of the array (five total)
    for (int i = 0; i < stringLength; i++){
        if (msgString[i] == ':'){
            colonCount++;
            colonIndex = i;
            if (colonCount == 1){
                strncpy(msgType, msgString + colonPrevious, i - colonPrevious);
                colonPrevious = colonIndex; 
                msgStruct.type = atoi((char*)msgType);
                // printf("convert string to message -- type at address %p or %p\n", msgType, &msgStruct.type);
            }
            else if (colonCount == 2){
                strncpy(msgSize, msgString + colonPrevious, i - colonPrevious);
                colonPrevious = colonIndex; 
                msgStruct.size = atoi((char*)msgSize);
                // printf("convert string to message -- size at address %p or %p\n", msgSize, &msgStruct.size);
            }
            else if (colonCount == 3){
                strncpy(msgSource, msgString + colonPrevious+1 , i - colonPrevious-1);
                colonPrevious = colonIndex; 
                strcpy(msgStruct.source, msgSource); // source is the username in this case, character array
                // printf("convert string to message -- source at address %p or %p\n", msgSource, &msgStruct.source);
                break;
            }
            
           /* else if(colonCount == 4){   
                strncpy(msgData, msgString + colonPrevious+1, i - colonPrevious-1);
                colonPrevious = colonIndex; 
                strcpy(msgStruct.data, msgData); 
                printf("convert string to message -- data at address %p or %p\n", msgData, &msgStruct.data);
                printf("message data %s\n", msgData);
            }*/

            //strncpy(msgData, msgString + colonPrevious+1, i - colonPrevious-1);
            // strcpy(msgStruct.data, msgData);
                   
        }
    }
    // strncpy(msgData, msgString + colonPrevious+1);
    strcpy(msgStruct.data, msgString + colonPrevious + 1);
    
    free(msgSize);
    free(msgType);
    free(msgData);
    free(msgSource);

    // strcpy(msgStruct.data, msgString + colonPrevious + 1);
    // strcpy(msgStruct.data, msgString + colonPrevious + 1);
    // printf("convert string to message -- data at address %p\n", &msgStruct.data);

    //strncpy(msgStruct.data, msgString + colonPrevious + 1, msgStruct.size); // check this one
    //printf("filedata: %s \n",packetStruct.filedata);
    

    return msgStruct;
}

// assumed str is declared outside as an array with large enough size
void messageToString(int type, int size, char* source, char* data, char* str){
    bzero(str, MAX_DATA);
    message send_msg;
    send_msg.type = type;
    send_msg.size = size;
    bzero(send_msg.data, MAX_DATA);
    bzero(send_msg.source, MAX_NAME);
    strcpy(send_msg.source, source);
    strcpy(send_msg.data, data);
    int str_temp = sprintf(str, "%u:%u:%s:%s",send_msg.type, send_msg.size, send_msg.source, send_msg.data);
    // printf("String from messageToString:\n %s", str);
    return;
}
