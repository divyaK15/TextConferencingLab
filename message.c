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


message convertStringToMessage(char* msgString, int sizeOfMessage){
    message msgStruct;
    
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
                memcpy((char*)msgHeaders[0], msgString + colonPrevious, i - colonPrevious);
                colonPrevious = colonIndex; 
                msgStruct.type = atoi((char*)msgHeaders[0]);
                //printf("total frag = %u\n", packetStruct.total_frag);
            }
            else if (colonCount == 2){
                msgHeaders[1] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(msgHeaders[1], msgString + colonPrevious + 1, i - colonPrevious - 1 );
                colonPrevious = colonIndex; 
                msgStruct.size = atoi(msgHeaders[1]);
                //printf("frag no: %u \n",packetStruct.frag_no);
            }
            else if (colonCount == 3){
                msgHeaders[2] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(msgHeaders[2], msgString + colonPrevious + 1, i - colonPrevious);
                colonPrevious = colonIndex; 
                strcpy(msgStruct.source, msgHeaders[2]); // source is the username in this case, character array
                //printf("size: %u \n",packetStruct.size);
            }
            else if(colonCount == 4){   
                msgHeaders[3] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(msgHeaders[3], msgString + colonPrevious + 1, i - colonPrevious - 1);
                colonPrevious = colonIndex; 
                strcpy(msgStruct.data, msgHeaders[3]);
                //printf("filename: %s\n",packetStruct.filename);
            }         
        }
    }
    memcpy(msgStruct.data, msgString + colonPrevious + 1, msgStruct.size); // check this one
    //printf("filedata: %s \n",packetStruct.filedata);

    return msgStruct;
}