CC=gcc
LDFLAGS=-pthread
all: server client sample_client
server: server.o message.o message.h
client: client.o message.o message.h
sample_client: sample_client.o 
clean: 
	rm -f *.o server client sample_client