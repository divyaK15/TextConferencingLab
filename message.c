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
#include "packet.h"


message convertStringToMessage(char* packetString, int sizeOfPacket){
    packet packetStruct;
    //packetStructptr_g = &packetStruct;
    
    // iterate through string and when we reach : its the next element of the struct
    int stringLength = sizeOfPacket;
    int colonCount = 0;
    int colonIndex = 0;
    int colonPrevious = 0;
    char* packet_headers[5]; 
    int packet_header_index = 0; 

    //the packet's fields should all be divided into parts of the array (five total)
    for (int i = 0; i < stringLength; i++){
        if (packetString[i] == ':'){
            colonCount++;
            colonIndex = i;
            //printf("strlength %d\n", stringLength);
            if (colonCount == 1){
                packet_headers[0] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(packet_headers[0], packetString + colonPrevious, i - colonPrevious);
                colonPrevious = colonIndex; 
                packetStruct.total_frag = atoi(packet_headers[0]);
                //printf("total frag = %u\n", packetStruct.total_frag);
            }
            else if (colonCount == 2){
                packet_headers[1] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(packet_headers[1], packetString + colonPrevious + 1, i - colonPrevious - 1 );
                colonPrevious = colonIndex; 
                packetStruct.frag_no = atoi(packet_headers[1]);
                //printf("frag no: %u \n",packetStruct.frag_no);
            }
            else if (colonCount == 3){
                packet_headers[2] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(packet_headers[2], packetString + colonPrevious + 1, i - colonPrevious);
                colonPrevious = colonIndex; 
                packetStruct.size = atoi(packet_headers[2]);
                //printf("size: %u \n",packetStruct.size);
            }
            else if(colonCount == 4){   
                packet_headers[3] = malloc(sizeof(char) * (i - colonPrevious));
                memcpy(packet_headers[3], packetString + colonPrevious + 1, i - colonPrevious - 1);
                colonPrevious = colonIndex; 
                packetStruct.filename = packet_headers[3];
                //printf("filename: %s\n",packetStruct.filename);
            }
            else{
                printf("Too many entries.\n");

            }
            
        }
    }
    memcpy(packetStruct.filedata, packetString + colonPrevious+ 1, packetStruct.size); // check this one
    //printf("filedata: %s \n",packetStruct.filedata);

    return packetStruct;
    //return packetStructptr_g;
}